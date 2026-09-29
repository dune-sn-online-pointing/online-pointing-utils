#!/usr/bin/env python3
"""Attach the deployed R4 per-event products (CT v80 score, ED v63 reco direction)
to the reference (e3p0 _matchfix) ES event list, and cache the CC rows.

The R4 slim tars hold, for scenario_4_weighted_ct, one row per LOADED cluster
(CC budget 3300 + ES budget 330) in the seed-42 shuffled order of
sample_loader.load_and_select_samples. The shuffle is inverted exactly
(np.random.seed(42); perm = np.random.permutation(n); shuffled[i] = original[perm[i]]),
the CC block precedes the ES block, and inside the ES block the rows are in loader
order (file index, then sorted match_id) -- i.e. exactly the order produced by
induction_threshold_scan.py. Verified on cat000623: event, match_id and energy agree
row by row.
"""
import argparse, io, os, tarfile
import numpy as np

R4 = "/eos/project-e/ep-nu/evilla/sn-online-pointing/pipeline-dev/ed_retrain/R4_v63_ownpdf_matchfix"
SCEN = "scenario_4_weighted_ct"


def read_cat(cat, r4=R4):
    tp = f"{r4}/cat{cat:06d}/cat{cat:06d}_scenarios.tar"
    with tarfile.open(tp) as t:
        names = [n for n in t.getnames() if n.startswith(SCEN) and n.endswith(".npz")]

        def load(tag):
            n = [x for x in names if tag in x][0]
            return np.load(io.BytesIO(t.extractfile(n).read()), allow_pickle=True)
        md = load("volumes.npz")["metadata"]
        cp = load("channel_predictions")
        rd = load("reco_directions")
    proba = np.asarray(cp["y_pred_proba"], float)
    dirs = np.asarray(rd["reco_dirs"], float)
    has = np.asarray(rd["has_reco"], bool)
    n = len(md)
    np.random.seed(42)
    perm = np.random.permutation(n)
    is_es = md[:, 3] == 1
    n_cc = int((~is_es).sum())
    es_shuf = np.array([i for i in range(n) if perm[i] >= n_cc], dtype=int)
    es_order = es_shuf[np.argsort(perm[es_shuf])]
    cc_order = np.array([i for i in range(n) if perm[i] < n_cc], dtype=int)
    return dict(md=md, proba=proba, dirs=dirs, has=has,
                es_order=es_order, cc_order=cc_order)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--scan", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--cats", default="623-672")
    a = ap.parse_args()
    lo, hi = (int(x) for x in a.cats.split("-"))
    P = np.load(a.scan, allow_pickle=True)["payload"][0]
    rows = P["matchfix"]["rows"]
    bycat = {}
    for r in rows:
        bycat.setdefault(r["cat"], []).append(r)

    out = {}
    for cat in range(lo, hi + 1):
        d = read_cat(cat)
        sr = bycat[cat]
        eo = d["es_order"]
        assert len(eo) == len(sr), (cat, len(eo), len(sr))
        ev = d["md"][eo, 0].astype(int)
        mid = d["md"][eo, 13].astype(int)
        assert (ev == np.array([r["event"] for r in sr])).all(), cat
        assert (mid == np.array([r["match_id"] for r in sr])).all(), cat
        out[cat] = dict(
            es_md=d["md"][eo], es_proba=d["proba"][eo], es_dirs=d["dirs"][eo],
            es_has=d["has"][eo], es_file_idx=np.array([r["file_idx"] for r in sr]),
            cc_md=d["md"][d["cc_order"]], cc_proba=d["proba"][d["cc_order"]],
            cc_dirs=d["dirs"][d["cc_order"]], cc_has=d["has"][d["cc_order"]],
        )
        print(f"cat{cat:06d}: ES {len(eo)}  CC {len(d['cc_order'])}", flush=True)
    np.savez_compressed(a.out, payload=np.array([out], dtype=object))
    print("wrote", a.out)


if __name__ == "__main__":
    main()
