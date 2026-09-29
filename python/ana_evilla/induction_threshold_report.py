#!/usr/bin/env python3
"""Aggregate the induction-threshold study: event counts, recovered-event quality,
pointing table with paired bootstrap intervals, and cost numbers."""
import argparse, glob, os
import numpy as np

TAGS = ["ref", "ind25", "ind20"]
SCEN = ["s2_perfect_ct_e3", "s6_perfect_ct_e5", "deployed_ct080_e5"]


def theta68(cos):
    return float(np.degrees(np.arccos(np.clip(np.quantile(np.asarray(cos), 0.32), -1.0, 1.0))))


def paired(a, b, n=2000, seed=23):
    rng = np.random.RandomState(seed)
    d = theta68(b) - theta68(a)
    bs = [theta68(b[i]) - theta68(a[i]) for i in (rng.randint(0, len(a), len(a)) for _ in range(n))]
    return d, float(np.percentile(bs, 16)), float(np.percentile(bs, 84))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--fits-dir", required=True)
    a = ap.parse_args()
    res = {}
    for f in sorted(glob.glob(os.path.join(a.fits_dir, "fits_*.npz"))):
        res.update(np.load(f, allow_pickle=True)["payload"][0])
    cats = sorted(res)
    print(f"cats: {len(cats)}")
    print("\n| scenario | set | mean n_sel | theta68 [deg] | median theta | d(theta68) vs ref [68% CI] |")
    print("|---|---|---|---|---|---|")
    for s in SCEN:
        base = np.array([res[c][("ref", s)]["cos"] for c in cats])
        for t in TAGS:
            cos = np.array([res[c][(t, s)]["cos"] for c in cats])
            n = np.array([res[c][(t, s)]["n_sel"] for c in cats], float)
            med = float(np.median(np.degrees(np.arccos(np.clip(cos, -1, 1)))))
            if t == "ref":
                cell = "-"
            else:
                d, lo, hi = paired(base, cos)
                cell = f"{d:+.2f} [{lo:+.2f}, {hi:+.2f}]"
            print(f"| {s} | {t} | {n.mean():.1f} | {theta68(cos):.2f} | {med:.2f} | {cell} |")


if __name__ == "__main__":
    main()
