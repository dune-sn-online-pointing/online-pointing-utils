#!/usr/bin/env python3
"""
Before/after validation of the three-plane matching fix (P3 + P4).

Works directly on the intermediate ``*_matched.root`` products written by
``src/app/match_clusters.cpp`` (no cluster images needed), so the same script can
be pointed at the production ``*_matched_clusters_*`` folder and at the new
``*_matched_clusters_*_matchfix`` folder and the two compared.

For every X-plane main-track cluster it evaluates exactly what the pointing
pipeline requires (``refactor-snop-pipeline/python/lib/sample_loader.py:261-266``):
the same ``match_id`` must sit on a **main-track** cluster in X *and* U *and* V.

Metrics produced
  * 3-plane main-track fraction, overall and in true-electron-energy bins
  * wrong-partner rate: the match_id exists in an induction plane but on a
    non-main cluster; split into "different event" / "non-MARLEY" / "MARLEY
    fragment of the same event"
  * orphan match_ids: carried by an X cluster but present in neither U nor V
    (the P4a effect)
  * match_type distribution (the P4b effect)
  * per-cat count of main-track clusters that would enter the pointing pipeline
    (3-plane matched and true particle energy above a threshold)

Read-only. Run with an LCG view sourced (uproot):
  source /cvmfs/sft.cern.ch/lcg/views/LCG_104/x86_64-el9-gcc11-opt/setup.sh
"""

import argparse
import json
import os
import sys
from collections import Counter, defaultdict

import numpy as np
import uproot

BRANCHES = ['event', 'is_main_cluster', 'match_id', 'match_type', 'cluster_id',
            'marley_tp_fraction', 'total_energy', 'true_particle_energy',
            'true_pdg', 'is_es_interaction', 'true_pos_x', 'true_pos_y', 'true_pos_z',
            'tp_detector']

E_BINS = [(3.0, 5.0), (5.0, 10.0), (10.0, 20.0), (20.0, 30.0), (30.0, 1e9)]
E_BIN_LABELS = ['3-5', '5-10', '10-20', '20-30', '30+']


def read_view(f, view, folder='clusters'):
    key = f"{folder}/clusters_tree_{view}"
    keys = [k.split(';')[0] for k in f.keys()]
    if key not in keys:
        return None
    t = f[key]
    have = [b for b in BRANCHES if b in t.keys()]
    d = t.arrays(have, library='np')
    n = len(d['event'])
    out = {}
    for b in BRANCHES:
        if b == 'tp_detector':
            # APA index of the cluster = detector of its first TP
            if b in d:
                out['apa'] = np.array([int(np.asarray(v)[0]) if len(np.asarray(v)) else -1
                                       for v in d[b]])
            else:
                out['apa'] = np.full(n, -1)
            continue
        if b in d:
            out[b] = np.asarray(d[b])
        else:
            out[b] = np.full(n, -1)
    out['n'] = n
    return out


def energy_bin(e):
    for i, (lo, hi) in enumerate(E_BINS):
        if lo <= e < hi:
            return i
    return None  # below 3 MeV or nan


def analyse_file(path):
    """Classify every X main-track cluster of one *_matched.root file."""
    res = dict(
        n_x_main=0, n_x_main_matched=0, n_3plane=0,
        n_orphan=0,                       # match_id in neither U nor V
        n_partner_bg_u=0, n_partner_bg_v=0, n_partner_bg_any=0,
        n_partner_diff_event=0, n_partner_non_marley=0, n_partner_marley_frag=0,
        n_partner_diff_apa=0,
        n_ok_partner_diff_event=0, n_ok_partner_non_marley=0, n_ok_partner_diff_apa=0,
        match_type_x=Counter(), match_type_u=Counter(), match_type_v=Counter(),
        ebin_tot=[0] * len(E_BINS), ebin_ok=[0] * len(E_BINS),
        n_below_3mev=0, n_ok_below_3mev=0,
        n_pointing=0,                     # 3-plane and E_true > 3 MeV
        n_pointing_es=0,                  # ... and is_es_interaction
        n_u_main=0, n_v_main=0,
        n_u_main_matched=0, n_v_main_matched=0,
    )
    with uproot.open(path) as f:
        cx = read_view(f, 'X')
        cu = read_view(f, 'U')
        cv = read_view(f, 'V')
    if cx is None:
        return res

    for view, cc, key in (('U', cu, 'u'), ('V', cv, 'v')):
        if cc is None:
            continue
        mm = cc['is_main_cluster'].astype(bool)
        res['n_%s_main' % key] = int(mm.sum())
        res['n_%s_main_matched' % key] = int((mm & (cc['match_id'] != -1)).sum())
        for mt, mid in zip(cc['match_type'], cc['match_id']):
            if mid != -1:
                res['match_type_%s' % key][int(mt)] += 1

    # match_id -> row index maps for the induction planes
    def build(cc):
        main_ids, all_rows = {}, {}
        if cc is None:
            return main_ids, all_rows
        for i in range(cc['n']):
            mid = int(cc['match_id'][i])
            if mid == -1:
                continue
            all_rows.setdefault(mid, i)
            if cc['is_main_cluster'][i]:
                main_ids.setdefault(mid, i)
        return main_ids, all_rows

    u_main, u_all = build(cu)
    v_main, v_all = build(cv)

    for i in range(cx['n']):
        if not cx['is_main_cluster'][i]:
            continue
        res['n_x_main'] += 1
        mid = int(cx['match_id'][i])
        e_true = float(cx['true_particle_energy'][i])
        ev = int(cx['event'][i])
        b = energy_bin(e_true)
        if b is None:
            res['n_below_3mev'] += 1
        else:
            res['ebin_tot'][b] += 1

        if mid != -1:
            res['n_x_main_matched'] += 1
            res['match_type_x'][int(cx['match_type'][i])] += 1
            if mid not in u_all and mid not in v_all:
                res['n_orphan'] += 1

        ok_u = mid != -1 and mid in u_main
        ok_v = mid != -1 and mid in v_main
        if ok_u and ok_v:
            res['n_3plane'] += 1
            if b is None:
                res['n_ok_below_3mev'] += 1
            else:
                res['ebin_ok'][b] += 1
            if e_true > 3.0:
                res['n_pointing'] += 1
                if cx['is_es_interaction'][i]:
                    res['n_pointing_es'] += 1

        # purity of the partners actually chosen (main or not)
        for cc, alls in ((cu, u_all), (cv, v_all)):
            if mid == -1 or cc is None or mid not in alls:
                continue
            j = alls[mid]
            if int(cc['event'][j]) != ev:
                res['n_ok_partner_diff_event'] += 1
            if float(cc['marley_tp_fraction'][j]) < 0.5:
                res['n_ok_partner_non_marley'] += 1
            if int(cc['apa'][j]) != int(cx['apa'][i]):
                res['n_ok_partner_diff_apa'] += 1

        # wrong partner: match_id present in the plane but on a non-main cluster
        bad = False
        for cc, mains, alls, key in ((cu, u_main, u_all, 'u'), (cv, v_main, v_all, 'v')):
            if mid == -1 or cc is None:
                continue
            if mid in alls and mid not in mains:
                res['n_partner_bg_%s' % key] += 1
                bad = True
                j = alls[mid]
                if int(cc['apa'][j]) != int(cx['apa'][i]):
                    res['n_partner_diff_apa'] += 1
                if int(cc['event'][j]) != ev:
                    res['n_partner_diff_event'] += 1
                elif float(cc['marley_tp_fraction'][j]) < 0.5:
                    res['n_partner_non_marley'] += 1
                else:
                    res['n_partner_marley_frag'] += 1
        if bad:
            res['n_partner_bg_any'] += 1
    return res


def merge(a, b):
    for k, v in b.items():
        if isinstance(v, Counter):
            a[k].update(v)
        elif isinstance(v, list):
            a[k] = [x + y for x, y in zip(a[k], v)]
        else:
            a[k] = a[k] + v
    return a


def empty_like():
    return dict(
        n_x_main=0, n_x_main_matched=0, n_3plane=0, n_orphan=0,
        n_partner_bg_u=0, n_partner_bg_v=0, n_partner_bg_any=0,
        n_partner_diff_event=0, n_partner_non_marley=0, n_partner_marley_frag=0,
        n_partner_diff_apa=0,
        n_ok_partner_diff_event=0, n_ok_partner_non_marley=0, n_ok_partner_diff_apa=0,
        match_type_x=Counter(), match_type_u=Counter(), match_type_v=Counter(),
        ebin_tot=[0] * len(E_BINS), ebin_ok=[0] * len(E_BINS),
        n_below_3mev=0, n_ok_below_3mev=0, n_pointing=0, n_pointing_es=0,
        n_u_main=0, n_v_main=0, n_u_main_matched=0, n_v_main_matched=0)


def analyse_folder(folder, prefix='es_', verbose=False):
    files = sorted(f for f in os.listdir(folder)
                   if f.startswith(prefix) and f.endswith('_matched.root'))
    tot = empty_like()
    tot['n_files'] = 0
    for fn in files:
        try:
            r = analyse_file(os.path.join(folder, fn))
        except Exception as exc:                      # noqa: BLE001
            print(f"    ! failed on {fn}: {exc}")
            continue
        merge(tot, r)
        tot['n_files'] += 1
        if verbose:
            print(f"    {fn}: X_main={r['n_x_main']} 3plane={r['n_3plane']}")
    return tot


def to_jsonable(d):
    out = {}
    for k, v in d.items():
        if isinstance(v, Counter):
            out[k] = {str(kk): int(vv) for kk, vv in sorted(v.items())}
        elif isinstance(v, list):
            out[k] = [int(x) for x in v]
        else:
            out[k] = int(v)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cats', required=True, help='e.g. 623-672')
    ap.add_argument('--base', default='/eos/user/e/evilla/dune/sn-tps/mixture_dev_samples')
    ap.add_argument('--suffix', default='', help='suffix of the matched_clusters folder, e.g. _matchfix')
    ap.add_argument('--conditions', default='tick3_ch2_min2_tot3_e3p0')
    ap.add_argument('--prefix', default='es_')
    ap.add_argument('--out', required=True)
    ap.add_argument('-v', '--verbose', action='store_true')
    args = ap.parse_args()

    cats = []
    for tok in args.cats.split(','):
        tok = tok.strip()
        if not tok:
            continue
        if '-' in tok:
            a, b = tok.split('-')
            cats.extend(range(int(a), int(b) + 1))
        else:
            cats.append(int(tok))

    per_cat = []
    combined = empty_like()
    combined['n_files'] = 0
    for c in cats:
        cs = f"cat{c:06d}"
        folder = os.path.join(args.base, cs,
                              f"{cs}_matched_clusters_{args.conditions}{args.suffix}")
        if not os.path.isdir(folder):
            print(f"{cs}: MISSING {folder}")
            continue
        r = analyse_folder(folder, prefix=args.prefix, verbose=args.verbose)
        r['cat'] = c
        r['folder'] = folder
        per_cat.append(r)
        n = r['n_x_main']
        print(f"{cs}: files={r['n_files']:3d} X_main={n:5d} "
              f"match_id!=-1={r['n_x_main_matched']:5d} "
              f"3plane={r['n_3plane']:5d} ({100.0*r['n_3plane']/n if n else 0:5.1f}%) "
              f"pointing(E>3)={r['n_pointing']:5d}")
        nf = r.pop('n_files')
        cat = r.pop('cat')
        fol = r.pop('folder')
        merge(combined, r)
        combined['n_files'] += nf
        r['n_files'], r['cat'], r['folder'] = nf, cat, fol

    n = combined['n_x_main']
    print("\n=== COMBINED ===")
    print(f"X main tracks           : {n}")
    print(f"match_id != -1          : {combined['n_x_main_matched']} "
          f"({100.0*combined['n_x_main_matched']/n if n else 0:.1f}%)")
    print(f"3-plane main-track match: {combined['n_3plane']} "
          f"({100.0*combined['n_3plane']/n if n else 0:.1f}%)")
    print(f"orphan match_ids        : {combined['n_orphan']} "
          f"({100.0*combined['n_orphan']/n if n else 0:.1f}%)")
    print(f"wrong partner (any)     : {combined['n_partner_bg_any']} "
          f"({100.0*combined['n_partner_bg_any']/n if n else 0:.1f}%)"
          f"  [U={combined['n_partner_bg_u']} V={combined['n_partner_bg_v']}]"
          f"  diff_event={combined['n_partner_diff_event']}"
          f" non_marley={combined['n_partner_non_marley']}"
          f" marley_frag={combined['n_partner_marley_frag']}"
          f" diff_apa={combined['n_partner_diff_apa']}")
    print(f"chosen partners (all)   : diff_event={combined['n_ok_partner_diff_event']}"
          f" non_marley={combined['n_ok_partner_non_marley']}"
          f" diff_apa={combined['n_ok_partner_diff_apa']}")
    print(f"pointing (3p, E>3 MeV)  : {combined['n_pointing']}  (ES flag: {combined['n_pointing_es']})")
    print("match_type X: " + str(dict(sorted(combined['match_type_x'].items()))))
    print("match_type U: " + str(dict(sorted(combined['match_type_u'].items()))))
    print("match_type V: " + str(dict(sorted(combined['match_type_v'].items()))))
    print("\nE_true bin |    N |  3-plane |   eff")
    for lab, t, o in zip(E_BIN_LABELS, combined['ebin_tot'], combined['ebin_ok']):
        print(f"{lab:>10} | {t:5d} | {o:8d} | {100.0*o/t if t else float('nan'):5.1f}%")
    print(f"{'<3':>10} | {combined['n_below_3mev']:5d} | {combined['n_ok_below_3mev']:8d} | "
          f"{100.0*combined['n_ok_below_3mev']/combined['n_below_3mev'] if combined['n_below_3mev'] else float('nan'):5.1f}%")

    out = dict(cats=cats, suffix=args.suffix, prefix=args.prefix,
               combined=to_jsonable(combined),
               per_cat=[dict(cat=r['cat'], folder=r['folder'], **to_jsonable(
                   {k: v for k, v in r.items() if k not in ('cat', 'folder')}))
                   for r in per_cat])
    with open(args.out, 'w') as fh:
        json.dump(out, fh, indent=1)
    print(f"\nwrote {args.out}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
