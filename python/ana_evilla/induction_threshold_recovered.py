#!/usr/bin/env python3
"""Tables for the recovered events: energy spectrum, E>5 survival, measured CT v80 pass
rate, and ED v63 direction quality, each compared with the standard (reference) events
in the same energy bins."""
import argparse, glob, os
import numpy as np

BINS = [0, 3, 4, 5, 6, 7, 8, 10, 12, 15, 20, 30, 100]


def burst_dirs(R):
    bd = {}
    for cat, d in R.items():
        v = np.concatenate([d["cc_md"][:, 15:18], d["es_md"][:, 15:18]])
        n = np.linalg.norm(v, axis=1, keepdims=True); n[n == 0] = 1
        m = (v / n).mean(axis=0)
        bd[cat] = m / np.linalg.norm(m)
    return bd


def norm(a):
    n = np.linalg.norm(a, axis=1, keepdims=True); n[n == 0] = 1
    return a / n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ref", required=True)
    ap.add_argument("--infer-dir", required=True)
    ap.add_argument("--ncats", type=int, default=50)
    a = ap.parse_args()
    R = np.load(a.ref, allow_pickle=True)["payload"][0]
    bd = burst_dirs(R)

    # reference (standard) events
    E, PR, CE, CB = [], [], [], []
    for cat, d in R.items():
        E.append(d["es_md"][:, 10]); PR.append(d["es_proba"])
        CE.append(np.sum(norm(d["es_dirs"]) * norm(d["es_md"][:, 7:10]), axis=1))
        CB.append(norm(d["es_dirs"]) @ bd[cat])
    ref = dict(E=np.concatenate(E), PR=np.concatenate(PR),
               ce=np.concatenate(CE), cb=np.concatenate(CB))

    var = {}
    for sfx in ("indcut25", "indcut20"):
        E, PR, CE, CB, OK = [], [], [], [], []
        for f in sorted(glob.glob(os.path.join(a.infer_dir, f"infer_{sfx}_*.npz"))):
            p = np.load(f, allow_pickle=True)["payload"][0]
            for cat, d in p.items():
                if d is None:
                    continue
                E.append(d["md"][:, 10]); PR.append(d["proba"]); OK.append(d["ct_ok"])
                CE.append(np.sum(norm(d["dirs"]) * norm(d["md"][:, 7:10]), axis=1))
                CB.append(norm(d["dirs"]) @ bd[cat])
        var[sfx] = dict(E=np.concatenate(E), PR=np.concatenate(PR), ok=np.concatenate(OK),
                        ce=np.concatenate(CE), cb=np.concatenate(CB))

    n = a.ncats
    print(f"\n## Recovered events ({n} cats, 330-ES budget per cat)\n")
    print("| set | recovered/burst | E>=5/burst | CT>=0.8 & E>=5 /burst | median E [MeV] |")
    print("|---|---|---|---|---|")
    for sfx, d in var.items():
        m5 = d["E"] >= 5
        mct = m5 & (d["PR"] >= 0.8) & d["ok"]
        print(f"| {sfx} | {len(d['E'])/n:.2f} | {m5.sum()/n:.2f} | {mct.sum()/n:.2f} | "
              f"{np.median(d['E']):.2f} |")

    for sfx, d in var.items():
        print(f"\n### {sfx}: recovered vs standard, per X-energy bin\n")
        print("| E_X [MeV] | n rec | n std | CT>=0.8 rec | CT>=0.8 std | "
              "<cos(reco,e-)> rec | std | <cos(reco,burst)> rec | std |")
        print("|---|---|---|---|---|---|---|---|---|")
        for i in range(len(BINS) - 1):
            mr = (d["E"] >= BINS[i]) & (d["E"] < BINS[i + 1])
            ms = (ref["E"] >= BINS[i]) & (ref["E"] < BINS[i + 1])
            if mr.sum() < 5 or ms.sum() < 5:
                continue
            print(f"| {BINS[i]:.0f}-{BINS[i+1]:.0f} | {mr.sum()} | {ms.sum()} | "
                  f"{(d['PR'][mr]>=0.8).mean()*100:.1f}% | {(ref['PR'][ms]>=0.8).mean()*100:.1f}% | "
                  f"{d['ce'][mr].mean():.3f} | {ref['ce'][ms].mean():.3f} | "
                  f"{d['cb'][mr].mean():.3f} | {ref['cb'][ms].mean():.3f} |")
        for lab, mr, ms in (("all", np.ones(len(d["E"]), bool), np.ones(len(ref["E"]), bool)),
                            ("E>=5", d["E"] >= 5, ref["E"] >= 5)):
            print(f"| **{lab}** | {mr.sum()} | {ms.sum()} | "
                  f"{(d['PR'][mr]>=0.8).mean()*100:.1f}% | {(ref['PR'][ms]>=0.8).mean()*100:.1f}% | "
                  f"{d['ce'][mr].mean():.3f} | {ref['ce'][ms].mean():.3f} | "
                  f"{d['cb'][mr].mean():.3f} | {ref['cb'][ms].mean():.3f} |")


if __name__ == "__main__":
    main()
