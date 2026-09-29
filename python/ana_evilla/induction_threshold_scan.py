#!/usr/bin/env python3
"""
Induction-threshold study: event bookkeeping for the 330-ES burst budget.

For each cat and each clustering variant (reference e3p0 all planes, X=3.0/ind=2.5,
X=3.0/ind=2.0) it replicates EXACTLY the pipeline selection of
refactor-snop-pipeline/python/lib/sample_loader.py (load_all_planes=True,
event_budget_mode='generated', events_per_file=40): an event enters the pipeline when
its X main-track cluster carries a match_id that is also carried by a *main-track*
cluster in U and in V.

Outputs one npz per variant with a per-selected-event row, plus per-cat
cluster-multiplicity and match-purity counters.

Cluster-image metadata columns (18):
  0 event, 1 is_marley, 2 is_main_track, 3 is_es, 4-6 true_pos, 7-9 true_mom,
  10 reco cluster energy [MeV], 11 true particle energy, 12 plane, 13 match_id,
  14 nu energy, 15-17 nu_mom
"""
import argparse
import glob
import os
import numpy as np

BASE = "/eos/user/e/evilla/dune/sn-tps/mixture_dev_samples"
COND = "tick3_ch2_min2_tot3_e3p0"
EVENTS_PER_FILE = 40


def variant_folder(cat, suffix):
    return f"{BASE}/cat{cat:06d}/cat{cat:06d}_cluster_images_{COND}_{suffix}"


def budget_mask(n_target, events_per_file=EVENTS_PER_FILE):
    n_full = n_target // events_per_file
    n_rest = n_target % events_per_file
    last_idx = n_full if n_rest > 0 else n_full - 1

    def in_budget(f_idx, ev):
        if f_idx < n_full:
            return True
        return f_idx == n_full and 1 <= ev <= n_rest
    return in_budget, last_idx


def scan_cat(cat, suffix, n_es=330, pattern="es_*_planeX.npz"):
    """Return (rows, counters) for one cat/variant."""
    folder = variant_folder(cat, suffix)
    xf = sorted(glob.glob(os.path.join(folder, "X", pattern)))
    if not xf:
        return None, None
    in_budget, last_idx = budget_mask(n_es)

    rows = []
    c = dict(n_clu_x=0, n_clu_u=0, n_clu_v=0,
             n_bg_clu_u=0, n_bg_clu_v=0, n_bg_clu_x=0,
             n_xmain=0, n_xmain_matched=0,
             n_u_partner=0, n_u_partner_main=0, n_u_partner_bg=0,
             n_v_partner=0, n_v_partner_main=0, n_v_partner_bg=0,
             n_files=0)

    for f_idx, xp in enumerate(xf):
        if f_idx > last_idx:
            break
        up = xp.replace("/X/", "/U/").replace("planeX", "planeU")
        vp = xp.replace("/X/", "/V/").replace("planeX", "planeV")
        if not (os.path.exists(up) and os.path.exists(vp)):
            continue
        mx = np.load(xp, allow_pickle=True)["metadata"]
        mu = np.load(up, allow_pickle=True)["metadata"]
        mv = np.load(vp, allow_pickle=True)["metadata"]
        c["n_files"] += 1
        c["n_clu_x"] += len(mx); c["n_clu_u"] += len(mu); c["n_clu_v"] += len(mv)
        c["n_bg_clu_x"] += int(np.sum(mx[:, 1] < 0.5))
        c["n_bg_clu_u"] += int(np.sum(mu[:, 1] < 0.5))
        c["n_bg_clu_v"] += int(np.sum(mv[:, 1] < 0.5))

        # --- purity bookkeeping over the X main tracks in the budget -------------
        # (any cluster carrying the same match_id in U/V is the chosen partner)
        u_by_mid, v_by_mid = {}, {}
        for i, m in enumerate(mu[:, 13]):
            if m != -1:
                u_by_mid.setdefault(int(m), i)
        for i, m in enumerate(mv[:, 13]):
            if m != -1:
                v_by_mid.setdefault(int(m), i)

        for i in range(len(mx)):
            if mx[i, 2] != 1:
                continue
            ev = int(mx[i, 0])
            if not in_budget(f_idx, ev):
                continue
            c["n_xmain"] += 1
            mid = int(mx[i, 13])
            if mid == -1:
                continue
            c["n_xmain_matched"] += 1
            for pl, dd, key in (("U", u_by_mid, "u"), ("V", v_by_mid, "v")):
                j = dd.get(mid)
                if j is None:
                    continue
                mm = mu if pl == "U" else mv
                c[f"n_{key}_partner"] += 1
                if mm[j, 2] == 1:
                    c[f"n_{key}_partner_main"] += 1
                if mm[j, 1] < 0.5:
                    c[f"n_{key}_partner_bg"] += 1

        # --- the pipeline selection (3-plane, main-track partners) ---------------
        is_main_x, is_main_u, is_main_v = mx[:, 2] == 1, mu[:, 2] == 1, mv[:, 2] == 1
        mid_x = {int(m): i for i, m in enumerate(mx[:, 13]) if is_main_x[i] and m != -1}
        mid_u = {int(m): i for i, m in enumerate(mu[:, 13]) if is_main_u[i] and m != -1}
        mid_v = {int(m): i for i, m in enumerate(mv[:, 13]) if is_main_v[i] and m != -1}
        for mid in sorted(set(mid_x) & set(mid_u) & set(mid_v)):
            ix, iu, iv = mid_x[mid], mid_u[mid], mid_v[mid]
            ev = int(mx[ix, 0])
            if not in_budget(f_idx, ev):
                continue
            rows.append(dict(
                cat=cat, file_idx=f_idx, event=ev, match_id=mid,
                e_reco_x=float(mx[ix, 10]), e_true=float(mx[ix, 11]),
                is_es=float(mx[ix, 3]), is_marley=float(mx[ix, 1]),
                mom=[float(mx[ix, 7]), float(mx[ix, 8]), float(mx[ix, 9])],
                numom=[float(mx[ix, 15]), float(mx[ix, 16]), float(mx[ix, 17])],
                e_reco_u=float(mu[iu, 10]), e_reco_v=float(mv[iv, 10]),
                idx_x=ix, idx_u=iu, idx_v=iv, xfile=xp,
            ))
    return rows, c


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cats", default="623-672")
    ap.add_argument("--suffixes", default="matchfix,indcut25,indcut20")
    ap.add_argument("--n-es", type=int, default=330)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()

    lo, hi = (int(x) for x in a.cats.split("-"))
    cats = list(range(lo, hi + 1))
    out = {}
    for sfx in a.suffixes.split(","):
        allrows, counters, missing = [], [], []
        for cat in cats:
            rows, c = scan_cat(cat, sfx, a.n_es)
            if rows is None:
                missing.append(cat)
                continue
            allrows += rows
            c["cat"] = cat
            counters.append(c)
        out[sfx] = dict(rows=allrows, counters=counters, missing=missing)
        print(f"{sfx}: {len(counters)} cats, {len(allrows)} selected events, "
              f"missing={missing}")
    np.savez_compressed(a.out, payload=np.array([out], dtype=object))
    print("wrote", a.out)


if __name__ == "__main__":
    main()
