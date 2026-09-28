#!/usr/bin/env python3
"""
Three-plane (X/U/V) main-track matching efficiency study.

Reads the per-plane cluster-image npz files produced by generate_cluster_arrays.py
(one npz per tpstream file per plane) and classifies, for every X-plane main-track
cluster, whether the same match_id also appears on a MAIN-TRACK cluster in U and V
(which is what refactor-snop-pipeline/python/lib/sample_loader.py requires when
load_all_planes=True), and if not, why.

Metadata columns (N, 18) float32:
  0 event, 1 is_marley, 2 is_main_track, 3 is_es, 4-6 true pos (x,y,z),
  7-9 true particle momentum, 10 cluster energy [MeV], 11 true particle energy [MeV],
  12 plane, 13 match_id (-1 = unmatched), 14 nu energy, 15-17 nu momentum

Read-only: it never writes into the sample folders.
"""

import argparse
import json
import os
import sys
from collections import Counter, defaultdict

import numpy as np

M_EVENT, M_MARLEY, M_MAIN, M_ES = 0, 1, 2, 3
M_POSX, M_POSY, M_POSZ = 4, 5, 6
M_ECLUS, M_ETRUE, M_PLANE, M_MID = 10, 11, 12, 13

APA_LENGTH_CM = 230.0
APA_GAP_CM = 2.4
APA_PITCH_CM = APA_LENGTH_CM + APA_GAP_CM

# per-plane failure statuses, in reporting order
STATUS_ORDER = ['ok', 'partner_bg', 'no_cluster', 'no_main', 'main_unmatched', 'main_other_id']


def default_images_dir(cat, base, suffix='', conditions='tick3_ch2_min2_tot3_e3p0'):
    cat_s = f"cat{cat:06d}"
    return os.path.join(base, cat_s, f"{cat_s}_cluster_images_{conditions}{suffix}")


def load_meta(path):
    with np.load(path) as d:
        return np.array(d['metadata'])


def event_main_info(meta_p, event):
    """(has_main_cluster_in_event, its energy, its match_id) for this plane/event."""
    if meta_p is None or len(meta_p) == 0:
        return False, float('nan'), None
    sel = (meta_p[:, M_EVENT].astype(np.int64) == event) & (meta_p[:, M_MAIN] > 0.5)
    if not sel.any():
        return False, float('nan'), None
    row = meta_p[np.where(sel)[0][0]]
    return True, float(row[M_ECLUS]), int(row[M_MID])


def partner_info(mid, meta_p):
    """(energy, is_marley) of the cluster that actually carries this match_id."""
    if meta_p is None or len(meta_p) == 0:
        return float('nan'), None
    w = np.where(meta_p[:, M_MID].astype(np.int64) == mid)[0]
    if len(w) == 0:
        return float('nan'), None
    return float(meta_p[w[0], M_ECLUS]), bool(meta_p[w[0], M_MARLEY] > 0.5)


def plane_status(mid, meta_p, event):
    """Why does match_id `mid` (from an X main track) not resolve to a main-track
    cluster in this induction plane?"""
    if meta_p is None or len(meta_p) == 0:
        return 'no_cluster'
    mids = meta_p[:, M_MID].astype(np.int64)
    is_main = meta_p[:, M_MAIN] > 0.5
    if mid in set(mids[is_main].tolist()):
        return 'ok'
    if mid in set(mids.tolist()):
        # match_id exists in this plane but sits on a non-main-track (background) cluster
        return 'partner_bg'
    ev = meta_p[:, M_EVENT].astype(np.int64) == event
    if not ev.any():
        return 'no_cluster'
    if not (ev & is_main).any():
        return 'no_main'
    ev_main_mids = mids[ev & is_main]
    if (ev_main_mids == -1).all():
        return 'main_unmatched'
    return 'main_other_id'


def analyse_triplet(px, pu, pv):
    meta_x = load_meta(px)
    meta_u = load_meta(pu) if os.path.exists(pu) else None
    meta_v = load_meta(pv) if os.path.exists(pv) else None

    recs = []
    if len(meta_x) == 0:
        return recs, meta_x, meta_u, meta_v

    main_x = np.where(meta_x[:, M_MAIN] > 0.5)[0]
    for i in main_x:
        row = meta_x[i]
        mid = int(row[M_MID])
        event = int(row[M_EVENT])
        rec = dict(
            event=event, mid=mid,
            e_reco=float(row[M_ECLUS]), e_true=float(row[M_ETRUE]),
            pos_x=float(row[M_POSX]), pos_y=float(row[M_POSY]), pos_z=float(row[M_POSZ]),
            is_marley=bool(row[M_MARLEY] > 0.5), is_es=bool(row[M_ES] > 0.5),
        )
        for pl, meta_p in (('u', meta_u), ('v', meta_v)):
            has, e_main, mid_main = event_main_info(meta_p, event)
            rec[pl + '_has_event_main'] = has
            rec[pl + '_e_main'] = e_main
            rec[pl + '_mid_main'] = mid_main
        if mid == -1:
            rec['u'] = rec['v'] = 'x_unmatched'
            rec['category'] = 'D_x_main_unmatched'
        else:
            su = plane_status(mid, meta_u, event)
            sv = plane_status(mid, meta_v, event)
            rec['u'], rec['v'] = su, sv
            for pl, meta_p in (('u', meta_u), ('v', meta_v)):
                e_p, marley_p = partner_info(mid, meta_p)
                rec[pl + '_e_partner'] = e_p
                rec[pl + '_partner_marley'] = marley_p
            if su == 'ok' and sv == 'ok':
                rec['category'] = 'MATCHED_3PLANE'
            else:
                bad = [s for s in (su, sv) if s != 'ok']
                # single overall label, worst (first in priority) failure
                prio = ['no_cluster', 'no_main', 'main_unmatched', 'main_other_id', 'partner_bg']
                worst = sorted(bad, key=lambda s: prio.index(s) if s in prio else 99)[0]
                tag = {'no_cluster': 'A_no_cluster_in_plane',
                       'no_main': 'B_cluster_but_not_main',
                       'main_unmatched': 'C1_main_exists_unmatched',
                       'main_other_id': 'C2_main_exists_other_id',
                       'partner_bg': 'C3_partner_is_background'}[worst]
                rec['category'] = tag
        recs.append(rec)
    return recs, meta_x, meta_u, meta_v


def analyse_cat(images_dir, prefix='es_', verbose=False):
    xdir = os.path.join(images_dir, 'X')
    if not os.path.isdir(xdir):
        return None
    xfiles = sorted(f for f in os.listdir(xdir)
                    if f.startswith(prefix) and f.endswith('_planeX.npz'))
    out = dict(images_dir=images_dir, n_files=0, recs=[],
               n_x_clusters=0, n_u_clusters=0, n_v_clusters=0,
               events_with_x_cluster=set(), events_with_x_main=set(),
               n_u_main=0, n_v_main=0, n_x_main=0,
               n_u_main_matched=0, n_v_main_matched=0)
    for xf in xfiles:
        px = os.path.join(xdir, xf)
        pu = os.path.join(images_dir, 'U', xf.replace('_planeX.npz', '_planeU.npz'))
        pv = os.path.join(images_dir, 'V', xf.replace('_planeX.npz', '_planeV.npz'))
        if not (os.path.exists(pu) and os.path.exists(pv)):
            if verbose:
                print(f"  missing U/V partner for {xf}, skipping")
            continue
        recs, mx, mu, mv = analyse_triplet(px, pu, pv)
        out['n_files'] += 1
        for r in recs:
            r['file'] = xf
        out['recs'].extend(recs)
        out['n_x_clusters'] += len(mx)
        out['n_u_clusters'] += 0 if mu is None else len(mu)
        out['n_v_clusters'] += 0 if mv is None else len(mv)
        if len(mx):
            out['events_with_x_cluster'] |= set(mx[:, M_EVENT].astype(int).tolist())
            mm = mx[:, M_MAIN] > 0.5
            out['n_x_main'] += int(mm.sum())
            out['events_with_x_main'] |= set(mx[mm, M_EVENT].astype(int).tolist())
        for meta, key in ((mu, 'u'), (mv, 'v')):
            if meta is None or len(meta) == 0:
                continue
            mm = meta[:, M_MAIN] > 0.5
            out[f'n_{key}_main'] += int(mm.sum())
            out[f'n_{key}_main_matched'] += int((mm & (meta[:, M_MID] != -1)).sum())
    out['events_with_x_cluster'] = len(out['events_with_x_cluster'])
    out['events_with_x_main'] = len(out['events_with_x_main'])
    return out


def summarise(res, label):
    recs = res['recs']
    n = len(recs)
    cats = Counter(r['category'] for r in recs)
    ustat = Counter(r['u'] for r in recs)
    vstat = Counter(r['v'] for r in recs)
    n_matched_any = sum(1 for r in recs if r['mid'] != -1)
    n_3p = cats.get('MATCHED_3PLANE', 0)
    s = dict(
        label=label, images_dir=res['images_dir'], n_files=res['n_files'],
        n_x_main=n, n_x_main_with_match_id=n_matched_any, n_3plane_main=n_3p,
        frac_match_id=n_matched_any / n if n else 0.0,
        frac_3plane=n_3p / n if n else 0.0,
        events_with_x_cluster=res['events_with_x_cluster'],
        events_with_x_main=res['events_with_x_main'],
        n_x_clusters=res['n_x_clusters'], n_u_clusters=res['n_u_clusters'],
        n_v_clusters=res['n_v_clusters'],
        n_u_main=res['n_u_main'], n_v_main=res['n_v_main'],
        n_u_main_matched=res['n_u_main_matched'], n_v_main_matched=res['n_v_main_matched'],
        categories=dict(cats), u_status=dict(ustat), v_status=dict(vstat),
    )
    return s


def energy_position_tables(recs):
    """Loss vs true electron energy and vs position."""
    ok = np.array([r['category'] == 'MATCHED_3PLANE' for r in recs])
    e = np.array([r['e_true'] for r in recs])
    ex = np.array([r['e_reco'] for r in recs])
    px = np.array([r['pos_x'] for r in recs])
    py = np.array([r['pos_y'] for r in recs])
    pz = np.array([r['pos_z'] for r in recs])
    tables = {}

    def binned(vals, edges, name):
        rows = []
        for lo, hi in zip(edges[:-1], edges[1:]):
            m = (vals >= lo) & (vals < hi)
            ntot = int(m.sum())
            npass = int((m & ok).sum())
            rows.append(dict(lo=float(lo), hi=float(hi), n=ntot, n_ok=npass,
                             eff=(npass / ntot if ntot else float('nan'))))
        tables[name] = rows

    binned(e, np.array([0, 5, 10, 15, 20, 25, 30, 40, 60, 1e9]), 'true_energy_MeV')
    binned(ex, np.array([0, 5, 10, 15, 20, 25, 30, 40, 60, 1e9]), 'reco_energy_X_MeV')
    binned(np.abs(px), np.array([0, 50, 100, 150, 200, 250, 300, 400, 1e9]), 'abs_drift_x_cm')
    binned(py, np.linspace(-700, 700, 15), 'y_cm')
    zin = np.mod(pz, APA_PITCH_CM)
    dedge = np.minimum(zin, APA_PITCH_CM - zin)
    binned(dedge, np.array([0, 2, 5, 10, 20, 40, 60, 80, 120]), 'dist_to_APA_edge_z_cm')
    binned(pz, np.array([0, 100, 200, 300, 400, 500, 600, 700, 1e9]), 'z_cm')
    return tables


def recovery_stats(recs):
    """Upper bound on what a main-track-preferring partner choice could recover."""
    n = len(recs)
    ok = [r for r in recs if r['category'] == 'MATCHED_3PLANE']
    lost = [r for r in recs if r['category'] != 'MATCHED_3PLANE']
    both_main = [r for r in lost if r.get('u_has_event_main') and r.get('v_has_event_main')]
    # of those, the ones where X is already matched (only the partner choice is wrong)
    both_main_xmatched = [r for r in both_main if r['mid'] != -1]
    bg_partner_marley = [r for r in recs
                         if r.get('u_partner_marley') is True or r.get('v_partner_marley') is True]
    wrong_partner = [r for r in recs if r['u'] == 'partner_bg' or r['v'] == 'partner_bg']
    wp_marley = [r for r in wrong_partner
                 if (r['u'] != 'partner_bg' or r.get('u_partner_marley')) and
                    (r['v'] != 'partner_bg' or r.get('v_partner_marley'))]
    wp_lower_e = []
    for r in wrong_partner:
        lower = True
        for pl in ('u', 'v'):
            if r[pl] == 'partner_bg':
                ep, em = r.get(pl + '_e_partner'), r.get(pl + '_e_main')
                if not (ep == ep and em == em and ep < em):
                    lower = False
        wp_lower_e.append(lower)
    return dict(
        n=n, n_ok=len(ok), n_lost=len(lost),
        lost_with_main_in_both_planes=len(both_main),
        lost_with_main_in_both_planes_and_x_matched=len(both_main_xmatched),
        n_wrong_partner=len(wrong_partner),
        n_wrong_partner_all_marley=len(wp_marley),
        n_wrong_partner_lower_energy=int(sum(wp_lower_e)),
        frac_if_partner_rule_fixed=(len(ok) + len(both_main_xmatched)) / n if n else 0.0,
        frac_ceiling_main_available=(len(ok) + len(both_main)) / n if n else 0.0,
    )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cats', type=str, required=True,
                    help='comma separated cat numbers or ranges, e.g. 623-632,1,100')
    ap.add_argument('--base', type=str,
                    default='/eos/project-e/ep-nu/evilla/sn-online-pointing/sn-burst-samples')
    ap.add_argument('--prefix', type=str, default='es_')
    ap.add_argument('--images-suffix', type=str, default='',
                    help="suffix appended to the cluster-image folder name, e.g. '_matchfix'")
    ap.add_argument('--conditions', type=str, default='tick3_ch2_min2_tot3_e3p0',
                    help='clustering-conditions part of the cluster-image folder name')
    ap.add_argument('--out', type=str, required=True, help='output json')
    ap.add_argument('--dump-recs', type=str, default=None, help='optional npz with per-track records')
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

    all_recs = []
    summaries = []
    for c in cats:
        d = default_images_dir(c, args.base, args.images_suffix, args.conditions)
        if not os.path.isdir(d):
            print(f"cat{c:06d}: no cluster-image folder ({d})")
            continue
        res = analyse_cat(d, prefix=args.prefix)
        if res is None or res['n_files'] == 0:
            print(f"cat{c:06d}: no {args.prefix}*_planeX.npz triplets")
            continue
        s = summarise(res, f"cat{c:06d}")
        s['cat'] = c
        summaries.append(s)
        for r in res['recs']:
            r['cat'] = c
        all_recs.extend(res['recs'])
        print(f"cat{c:06d}: files={s['n_files']:3d} X_main={s['n_x_main']:5d} "
              f"match_id!=-1={s['n_x_main_with_match_id']:5d} ({100*s['frac_match_id']:5.1f}%) "
              f"3plane={s['n_3plane_main']:5d} ({100*s['frac_3plane']:5.1f}%)")

    out = dict(cats=cats, prefix=args.prefix, images_suffix=args.images_suffix,
               summaries=summaries)
    if all_recs:
        out['combined'] = dict(
            n_x_main=len(all_recs),
            categories=dict(Counter(r['category'] for r in all_recs)),
            u_status=dict(Counter(r['u'] for r in all_recs)),
            v_status=dict(Counter(r['v'] for r in all_recs)),
            uv_status=dict(Counter(f"U:{r['u']}|V:{r['v']}" for r in all_recs)),
            tables=energy_position_tables(all_recs),
            recovery=recovery_stats(all_recs),
        )
    with open(args.out, 'w') as f:
        json.dump(out, f, indent=1)
    print(f"\nwrote {args.out}")

    if args.dump_recs and all_recs:
        keys_f = ['e_true', 'e_reco', 'pos_x', 'pos_y', 'pos_z']
        np.savez_compressed(
            args.dump_recs,
            cat=np.array([r['cat'] for r in all_recs]),
            event=np.array([r['event'] for r in all_recs]),
            mid=np.array([r['mid'] for r in all_recs]),
            category=np.array([r['category'] for r in all_recs]),
            u=np.array([r['u'] for r in all_recs]),
            v=np.array([r['v'] for r in all_recs]),
            **{k: np.array([r[k] for r in all_recs], dtype=np.float32) for k in keys_f})
        print(f"wrote {args.dump_recs}")


if __name__ == '__main__':
    sys.exit(main())
