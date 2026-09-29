#!/usr/bin/env python3
"""CC-side probe: how many CC main-track clusters pass the pipeline's 3-plane
main-track requirement at e3p0 vs induction 2.5 / 2.0, and how many extra induction
clusters are produced. Counted directly on the matched_clusters ROOT trees, so no
cluster images are needed."""
import argparse, glob, os
import numpy as np
import uproot

BASE = "/eos/user/e/evilla/dune/sn-tps/mixture_dev_samples"
BR = ["event", "is_main_cluster", "match_id", "marley_tp_fraction", "total_energy"]


def scan(cat, sfx):
    folder = f"{BASE}/cat{cat:06d}/cat{cat:06d}_matched_clusters_ccprobe_{sfx}"
    files = sorted(glob.glob(folder + "/cc_*_matched.root"))
    c = dict(n_files=len(files), n_main_x=0, n_3plane=0,
             n_clu_u=0, n_clu_v=0, n_clu_x=0, n_bg_u=0, n_bg_v=0)
    for fp in files:
        try:
            f = uproot.open(fp)
            d = {pl: f[f"clusters/clusters_tree_{pl}"].arrays(BR, library="np") for pl in "UVX"}
        except Exception as e:
            print(f"  skip {fp}: {e}")
            continue
        for pl in "UVX":
            c[f"n_clu_{pl.lower()}"] += len(d[pl]["event"])
        for pl in "UV":
            c[f"n_bg_{pl.lower()}"] += int(np.sum(d[pl]["marley_tp_fraction"] <= 0.5))
        mx, mu, mv = (d[pl] for pl in "XUV")
        idx = {}
        for pl, dd in (("X", mx), ("U", mu), ("V", mv)):
            idx[pl] = {int(v) for i, v in enumerate(dd["match_id"])
                       if dd["is_main_cluster"][i] and v != -1}
        c["n_main_x"] += int(np.sum(mx["is_main_cluster"]))
        c["n_3plane"] += len(idx["X"] & idx["U"] & idx["V"])
    return c


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cats", default="623-632")
    a = ap.parse_args()
    lo, hi = (int(x) for x in a.cats.split("-"))
    print("| variant | cc files | CC X mains | CC 3-plane | eff [%] | U clusters | V clusters | U bg | V bg |")
    print("|---|---|---|---|---|---|---|---|---|")
    for sfx in ("e3p0", "indcut25", "indcut20"):
        tot = None
        for cat in range(lo, hi + 1):
            c = scan(cat, sfx)
            tot = c if tot is None else {k: tot[k] + c[k] for k in c}
        n = hi - lo + 1
        print(f"| {sfx} | {tot['n_files']} | {tot['n_main_x']} | {tot['n_3plane']} | "
              f"{tot['n_3plane']/max(tot['n_main_x'],1)*100:.1f} | "
              f"{tot['n_clu_u']/n:.0f} | {tot['n_clu_v']/n:.0f} | "
              f"{tot['n_bg_u']/n:.0f} | {tot['n_bg_v']/n:.0f} |")


if __name__ == "__main__":
    main()
