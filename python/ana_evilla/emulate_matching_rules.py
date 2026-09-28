#!/usr/bin/env python3
"""
Reproduce the production X-U-V cluster matching from the intermediate ROOT products
and measure what alternative *ambiguity-resolution rules* would give, without
touching any production code or JSON.

The production rule (src/app/match_clusters.cpp:261-352) is:
  clusters of each view are stable-sorted by (earliest TDC time, event, cluster_id);
  for every X cluster with is_main_cluster, scan U (then V) from a binary-search
  starting point and keep the FIRST cluster that
     - overlaps in time within +-time_tolerance (JSON ticks * 32 TDC),
     - has the same event number,
     - sits on the same APA.
  There is no preference for the main-track/most energetic induction cluster.

Run with an LCG view sourced (uproot needed):
  source /cvmfs/sft.cern.ch/lcg/views/LCG_104/x86_64-el9-gcc11-opt/setup.sh
"""

import argparse
import glob
import json
import os
from collections import Counter, defaultdict

import numpy as np
import uproot

APA_TOTAL_CHANNELS = 800 * 2 + 960
TDC_PER_TPC = 32
# parameters/conversion.dat
ADC_TO_MEV = {'X': 3600.0, 'U': 900.0, 'V': 900.0}

BRANCHES = ['event', 'is_main_cluster', 'match_id', 'match_type', 'cluster_id',
            'marley_tp_fraction', 'total_energy', 'total_charge', 'true_particle_energy',
            'true_pdg', 'tp_time_start', 'tp_samples_over_threshold',
            'tp_detector', 'tp_detector_channel', 'true_pos_x', 'true_pos_y', 'true_pos_z']


def read_view(f, view, folder='clusters'):
    key = f"{folder}/clusters_tree_{view}"
    if key not in [k.split(';')[0] for k in f.keys()]:
        return []
    t = f[key]
    have = [b for b in BRANCHES if b in t.keys()]
    d = t.arrays(have, library='np')
    n = len(d['event'])
    out = []
    for i in range(n):
        ts = np.asarray(d['tp_time_start'][i], dtype=np.int64)
        sot = np.asarray(d['tp_samples_over_threshold'][i], dtype=np.int64)
        if len(ts) == 0:
            continue
        ch = np.asarray(d['tp_detector_channel'][i], dtype=np.int64)
        out.append(dict(
            idx=i, event=int(d['event'][i]),
            is_main=bool(d['is_main_cluster'][i]),
            match_id=int(d['match_id'][i]),
            match_type=int(d['match_type'][i]) if 'match_type' in d else -1,
            cluster_id=int(d['cluster_id'][i]),
            marley=float(d['marley_tp_fraction'][i]),
            energy=float(d['total_energy'][i]),
            charge=float(d['total_charge'][i]),
            e_true=float(d['true_particle_energy'][i]),
            pdg=int(d['true_pdg'][i]) if 'true_pdg' in d else 0,
            tmin=int(ts.min()), tmax=int((ts + sot).max()),
            tmean=float(ts.mean()),
            apa=int(ch[0] // APA_TOTAL_CHANNELS),
            pos=(float(d['true_pos_x'][i]), float(d['true_pos_y'][i]), float(d['true_pos_z'][i])),
        ))
    return out


def sort_like_production(clusters):
    # stable sort by (tmin, event, cluster_id) - match_clusters.cpp:136-211
    return sorted(clusters, key=lambda c: (c['tmin'], c['event'], c['cluster_id']))


def times_overlap(a, b, tol):
    return (a['tmax'] + tol >= b['tmin']) and (b['tmax'] + tol >= a['tmin'])


def candidates_for(x, ind_sorted, tol):
    """All induction clusters passing the production gates, in production scan order."""
    out = []
    for c in ind_sorted:
        if c['tmin'] > x['tmax'] + tol:
            break
        if not times_overlap(c, x, tol):
            continue
        if c['event'] != x['event']:
            continue
        if c['apa'] != x['apa']:
            continue
        out.append(c)
    return out


RULES = {
    'current_first_in_time': lambda cands, x: cands[0],
    'prefer_main_track': lambda cands, x: next((c for c in cands if c['is_main']), cands[0]),
    'max_energy': lambda cands, x: max(cands, key=lambda c: c['energy']),
    'closest_time_centre': lambda cands, x: min(cands, key=lambda c: abs(c['tmean'] - x['tmean'])),
}


def energy_mev(c, view):
    return c['charge'] / ADC_TO_MEV[view]


def analyse_file(path, tol_tpc_ticks=10, verbose=False, energy_cut=None, uv_only=False):
    tol = tol_tpc_ticks * TDC_PER_TPC
    with uproot.open(path) as f:
        cu = read_view(f, 'U')
        cv = read_view(f, 'V')
        cx = read_view(f, 'X')
        du = read_view(f, 'U', 'discarded')
        dv = read_view(f, 'V', 'discarded')
        dx = read_view(f, 'X', 'discarded')

    # emulate a different energy_cut: rebuild the accepted sets from accepted+discarded
    if energy_cut is not None:
        cu = [c for c in cu + du if energy_mev(c, 'U') >= energy_cut]
        cv = [c for c in cv + dv if energy_mev(c, 'V') >= energy_cut]
        if not uv_only:
            cx = [c for c in cx + dx if energy_mev(c, 'X') >= energy_cut]

    su, sv = sort_like_production(cu), sort_like_production(cv)
    res = dict(path=os.path.basename(path),
               n_x=len(cx), n_u=len(cu), n_v=len(cv),
               n_x_main=sum(c['is_main'] for c in cx),
               n_disc_u_main=sum(c['is_main'] for c in du),
               n_disc_v_main=sum(c['is_main'] for c in dv),
               n_disc_x_main=sum(c['is_main'] for c in dx),
               rules={r: 0 for r in RULES}, n_considered=0,
               reproduced_u=0, reproduced_v=0, checked_u=0, checked_v=0,
               n_no_cand_u=0, n_no_cand_v=0,
               n_main_cand_u=0, n_main_cand_v=0,
               ncand_hist=Counter(), picked_rank_of_main=Counter(),
               ex_time_order=[])
    # events whose induction main track was thrown away by the energy cut
    disc_main_events = {'U': {c['event'] for c in du if c['is_main']},
                        'V': {c['event'] for c in dv if c['is_main']}}
    res['events_disc_main_u'] = len(disc_main_events['U'])
    res['events_disc_main_v'] = len(disc_main_events['V'])

    fate = Counter()
    ev_acc = {'U': {c['event'] for c in cu if c['is_main']},
              'V': {c['event'] for c in cv if c['is_main']}}
    ev_disc = {'U': {c['event'] for c in du if c['is_main']},
               'V': {c['event'] for c in dv if c['is_main']}}
    ev_any = {'U': {c['event'] for c in cu + du}, 'V': {c['event'] for c in cv + dv}}
    disc_main_e = {'U': [energy_mev(c, 'U') for c in du if c['is_main']],
                   'V': [energy_mev(c, 'V') for c in dv if c['is_main']]}
    res['disc_main_energy'] = disc_main_e

    per_track = []
    for x in cx:
        if not x['is_main']:
            continue
        res['n_considered'] += 1
        cand_u = candidates_for(x, su, tol)
        cand_v = candidates_for(x, sv, tol)
        res['ncand_hist'][(len(cand_u), len(cand_v))] += 1
        if not cand_u:
            res['n_no_cand_u'] += 1
        if not cand_v:
            res['n_no_cand_v'] += 1
        has_main_u = any(c['is_main'] for c in cand_u)
        has_main_v = any(c['is_main'] for c in cand_v)
        res['n_main_cand_u'] += has_main_u
        res['n_main_cand_v'] += has_main_v

        # cross-check the emulation against the stored match_id of the production run
        if cand_u:
            picked = cand_u[0]
            res['checked_u'] += 1
            if picked['match_id'] == x['match_id'] and x['match_id'] != -1:
                res['reproduced_u'] += 1
        if cand_v:
            picked = cand_v[0]
            res['checked_v'] += 1
            if picked['match_id'] == x['match_id'] and x['match_id'] != -1:
                res['reproduced_v'] += 1

        # where does the main induction cluster sit in the production scan order?
        for cands, key in ((cand_u, 'U'), (cand_v, 'V')):
            for rank, c in enumerate(cands):
                if c['is_main']:
                    res['picked_rank_of_main'][(key, rank)] += 1
                    if rank > 0 and len(res['ex_time_order']) < 25:
                        res['ex_time_order'].append(dict(
                            plane=key, event=x['event'], x_energy=x['energy'],
                            picked_tmin=cands[0]['tmin'], picked_E=cands[0]['energy'],
                            picked_marley=cands[0]['marley'], picked_main=cands[0]['is_main'],
                            main_tmin=c['tmin'], main_E=c['energy'], rank=rank,
                            dt_tdc=c['tmin'] - cands[0]['tmin']))
                    break

        for pl in ('U', 'V'):
            if x['event'] in ev_acc[pl]:
                fate[pl + '_main_accepted'] += 1
            elif x['event'] in ev_disc[pl]:
                fate[pl + '_main_cut_by_energy_cut'] += 1
            elif x['event'] in ev_any[pl]:
                fate[pl + '_clusters_but_no_main'] += 1
            else:
                fate[pl + '_no_cluster_at_all'] += 1

        rec = dict(event=x['event'], e_true=x['e_true'], e_reco=x['energy'], pos=x['pos'])
        for rname, rule in RULES.items():
            ok = bool(cand_u) and bool(cand_v)
            if ok:
                pu, pv = rule(cand_u, x), rule(cand_v, x)
                ok = pu['is_main'] and pv['is_main']
            res['rules'][rname] += ok
            rec[rname] = ok
        per_track.append(rec)
    res['per_track'] = per_track
    res['fate'] = dict(fate)
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--folder', required=True, help='folder with *_matched.root (or *_clusters.root)')
    ap.add_argument('--pattern', default='*_matched.root')
    ap.add_argument('--tol-ticks', type=int, default=10, help='time_tolerance_ticks (TPC ticks) as in the JSON')
    ap.add_argument('--energy-cut', type=float, default=None,
                    help='emulate a different energy_cut [MeV] by rebuilding the accepted set '
                         'from accepted+discarded clusters (3.0 reproduces production)')
    ap.add_argument('--energy-cut-uv-only', action='store_true',
                    help='apply --energy-cut to the induction planes only (keeps the X main-track '
                         'sample, and therefore the denominator, unchanged)')
    ap.add_argument('--out', default=None)
    args = ap.parse_args()

    files = sorted(glob.glob(os.path.join(args.folder, args.pattern)))
    if not files:
        raise SystemExit(f"no files matching {args.pattern} in {args.folder}")

    agg = defaultdict(int)
    fate = Counter()
    disc_e = {'U': [], 'V': []}
    rules = Counter()
    rank = Counter()
    ncand = Counter()
    examples = []
    per_track = []
    for p in files:
        r = analyse_file(p, args.tol_ticks, energy_cut=args.energy_cut,
                         uv_only=args.energy_cut_uv_only)
        for k in ['n_x', 'n_u', 'n_v', 'n_x_main', 'n_considered', 'n_no_cand_u', 'n_no_cand_v',
                  'n_main_cand_u', 'n_main_cand_v', 'reproduced_u', 'reproduced_v',
                  'checked_u', 'checked_v', 'n_disc_u_main', 'n_disc_v_main', 'n_disc_x_main',
                  'events_disc_main_u', 'events_disc_main_v']:
            agg[k] += r[k]
        rules.update({k: v for k, v in r['rules'].items()})
        rank.update({f"{k[0]}_rank{k[1]}": v for k, v in r['picked_rank_of_main'].items()})
        ncand.update({f"U{k[0]}_V{k[1]}": v for k, v in r['ncand_hist'].items()})
        fate.update(r['fate'])
        for pl in ('U', 'V'):
            disc_e[pl].extend(r['disc_main_energy'][pl])
        examples.extend(r['ex_time_order'][:5])
        per_track.extend(r['per_track'])
        print(f"{r['path']}: X_main={r['n_x_main']:4d} " +
              ' '.join(f"{k}={v}" for k, v in r['rules'].items()))

    n = agg['n_considered']
    print(f"\n=== {len(files)} files, {n} X main tracks ===")
    print(f"time tolerance: {args.tol_ticks} TPC ticks = {args.tol_ticks*TDC_PER_TPC} TDC ticks")
    for k, v in rules.most_common():
        print(f"  3-plane main-track match, rule {k:24s}: {v:5d}  {100*v/n:5.1f}%")
    print(f"  no U candidate at all : {agg['n_no_cand_u']:5d} ({100*agg['n_no_cand_u']/n:.1f}%)")
    print(f"  no V candidate at all : {agg['n_no_cand_v']:5d} ({100*agg['n_no_cand_v']/n:.1f}%)")
    print(f"  main U among candidates: {agg['n_main_cand_u']:5d} ({100*agg['n_main_cand_u']/n:.1f}%)")
    print(f"  main V among candidates: {agg['n_main_cand_v']:5d} ({100*agg['n_main_cand_v']/n:.1f}%)")
    print(f"  emulation reproduces production pick: U {agg['reproduced_u']}/{agg['checked_u']}, "
          f"V {agg['reproduced_v']}/{agg['checked_v']}")
    print(f"  main-track clusters thrown away by the energy cut (discarded tree): "
          f"U={agg['n_disc_u_main']} V={agg['n_disc_v_main']} X={agg['n_disc_x_main']}")
    print("  fate of the induction main-track cluster of the same event:")
    for k in sorted(fate):
        print(f"    {k:32s} {fate[k]:5d}  {100*fate[k]/n:5.1f}%")
    for pl in ('U', 'V'):
        arr = np.array(disc_e[pl])
        if len(arr):
            print(f"  energy [MeV] of {pl} MAIN clusters removed by the energy cut: n={len(arr)} "
                  f"median={np.median(arr):.2f} q90={np.quantile(arr,0.9):.2f} max={arr.max():.2f}")
    print("  rank of the main induction cluster in the production scan order:")
    for k, v in sorted(rank.items()):
        print(f"    {k:12s} {v:5d}")
    print("  example events where the first-in-time pick is not the main cluster:")
    for e in examples[:12]:
        print("   ", e)

    if args.out:
        with open(args.out, 'w') as f:
            json.dump(dict(files=len(files), n_x_main=n, tol_ticks=args.tol_ticks,
                           energy_cut=args.energy_cut, fate=dict(fate),
                           rules=dict(rules), agg=dict(agg), rank=dict(rank),
                           ncand=dict(ncand.most_common(20)), examples=examples,
                           per_track=per_track), f, indent=1)
        print(f"\nwrote {args.out}")


if __name__ == '__main__':
    main()
