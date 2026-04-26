#!/usr/bin/env python3
"""
get_3d_clouds.py

Lightweight matched-cluster processor:
- Loads matched clusters (same loader as plotting script)
- Runs 3D candidate generation + reconstruction
- Writes ONE .npz containing per-(event_id, match_id) metadata + ragged 3D clouds

NPZ contents (always):
  event_id            int64   (M,)
  match_id            int64   (M,)
  collection_adc      float64 (M,)   # X-plane (collection) total "ADC" proxy from TP integrals
  truth_xyz           float64 (M,3)  # (x,y,z) NaN if missing
  truth_mom_xyz       float64 (M,3)  # (px,py,pz) NaN if missing
  cloud_data          float32 (N,4)  # concatenated (x,y,z,amp)
  cloud_offsets       int64   (M+1,) # slice for group i: cloud_data[offsets[i]:offsets[i+1]]

Optional (if --save-all-hypotheses):
  hyp_data            float32 (K,3)  # concatenated (x,y,z) hypotheses
  hyp_offsets         int64   (M+1,)

Example:
  ./get_3d_clouds.py --clusters-file matched_clusters.root --out clouds.npz
  ./get_3d_clouds.py --clusters-dir ./out -n 10 --out clouds.npz --save-all-hypotheses
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import numpy as np

from load_matched_clusters import MatchedClustersLoader, ClusterItem, MatchedGroup
import reco_3d

# Imports for debugging
from space_transformations import xyz_to_UVX_wires_lut
from reco_3d import U_y_sorted, U_u_sorted, V_y_sorted, V_v_sorted


def _resolve_input_files(args) -> List[str]:
    """
    Returns an ordered list of ROOT files to process.
    Priority:
      1) --clusters-file (single file)
      2) --clusters-dir (directory; take first n ROOT-like files)
      3) JSON config: clusters_file or clusters_dir (+ optional n_files)
    """
    cfg: Dict[str, Any] = {}
    if args.json:
        with open(args.json, "r") as f:
            cfg = json.load(f)

    clusters_file = args.clusters_file or cfg.get("clusters_file")
    clusters_dir = args.clusters_dir or cfg.get("clusters_dir")
    n_files = args.n_files if args.n_files is not None else cfg.get("n_files", 1)

    if clusters_file:
        p = Path(clusters_file)
        if not p.is_file():
            raise SystemExit(f"ERROR: --clusters-file not found: {p}")
        return [str(p)]

    if clusters_dir:
        d = Path(clusters_dir)
        if not d.is_dir():
            raise SystemExit(f"ERROR: --clusters-dir not found or not a directory: {d}")

        patterns = ("*.root", "*.ROOT")
        files: List[Path] = []
        for pat in patterns:
            files.extend(d.glob(pat))

        files = sorted(set(files), key=lambda p: str(p))
        if not files:
            raise SystemExit(f"ERROR: No ROOT files found in directory: {d}")

        n = int(n_files)
        if n <= 0:
            raise SystemExit("ERROR: -n must be >= 1")

        return [str(p) for p in files[:n]]

    raise SystemExit("ERROR: provide --clusters-file or --clusters-dir (or JSON with clusters_file/clusters_dir).")


def _safe_tp_count(c: Optional[ClusterItem]) -> int:
    if c is None:
        return 0
    return min(len(c.ch), len(c.tstart), len(c.sot))


def _collection_adc_from_xplane(group: MatchedGroup) -> float:
    """
    Lightweight proxy for "collection ADC" from the X-plane cluster.

    Uses sum(adc_integral[i]) when available, else falls back to approx peak*samples/2.
    Mirrors the fallback idea used in your plotting histogram builder.
    """
    c = group.get("X")
    n = _safe_tp_count(c)
    if c is None or n <= 0:
        return 0.0

    total = 0.0
    for i in range(n):
        tot = float(c.sot[i]) if i < len(c.sot) else 0.0
        if tot <= 0:
            continue

        if hasattr(c, "adc_integral") and i < len(c.adc_integral):
            try:
                total += float(c.adc_integral[i])
                continue
            except Exception:
                pass

        peak = 200.0
        if hasattr(c, "adc_peak") and i < len(c.adc_peak):
            try:
                peak = float(c.adc_peak[i])
            except Exception:
                peak = 200.0

        total += peak * tot / 2.0

    return float(total)


def _truth_xyz(group: MatchedGroup) -> np.ndarray:
    vals = [group.true_x, group.true_y, group.true_z]
    out = np.full((3,), np.nan, dtype=np.float64)
    for k, v in enumerate(vals):
        if v is None:
            continue
        try:
            out[k] = float(v)
        except Exception:
            out[k] = np.nan
    return out


def _truth_mom_xyz(group: MatchedGroup) -> np.ndarray:
    vals = [group.true_mom_x, group.true_mom_y, group.true_mom_z]
    out = np.full((3,), np.nan, dtype=np.float64)
    for k, v in enumerate(vals):
        if v is None:
            continue
        try:
            out[k] = float(v)
        except Exception:
            out[k] = np.nan
    return out


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Produce 3D clouds from matched clusters (no plotting), write one .npz"
    )

    parser.add_argument("--clusters-file", help="Input matched_clusters ROOT file")
    parser.add_argument("--clusters-dir", help="Directory containing matched_clusters ROOT files")
    parser.add_argument("-n", "--n-files", type=int, default=None,
                        help="If using --clusters-dir, load the first N files (default: 1, or json n_files)")
    parser.add_argument("-j", "--json", help="Optional JSON config (can provide clusters_file or clusters_dir)")

    parser.add_argument("--out", required=True, help="Output .npz path (single file for all groups)")

    # Keep the same reco knobs
    parser.add_argument("--keep-negative", action="store_true")
    parser.add_argument("--no-tdc-to-tpc", action="store_true")
    parser.add_argument("--tdc-factor", type=float, default=32.0)

    parser.add_argument("--omp-iters", type=int, default=30, help="Maximum OMP iterations")
    parser.add_argument("--omp-max-reuse", type=int, default=1, help="Max reuse per (tick,wire) bin")
    parser.add_argument("--omp-stop-l1", type=float, default=1e-6, help="OMP stop threshold (L1 residual)")

    parser.add_argument("--use-peak-tick", action="store_true",
                        help="Use peak-tick charge placement (instead of uniform TOT distribution)")
    parser.add_argument("--expand-dt", type=int, default=1,
                        help="Timing tolerance in ticks for TP placement (±expand_dt)")

    parser.add_argument("--save-all-hypotheses", action="store_true",
                        help="Also store all UVX-consistent hypotheses (ragged) in the NPZ")

    parser.add_argument("--skip-event-id", type=int, default=11168,
                        help="Skip a specific event_id (default matches your plotting script behavior)")
    parser.add_argument("-v", "--verbose", action="store_true")

    args = parser.parse_args()
    files = _resolve_input_files(args)

    # Accumulators (per group)
    ev_ids: List[int] = []
    match_ids: List[int] = []
    col_adcs: List[float] = []
    truth_xyz_list: List[np.ndarray] = []
    truth_mom_list: List[np.ndarray] = []

    # Ragged clouds (concatenated + offsets)
    cloud_rows: List[np.ndarray] = []
    cloud_offsets: List[int] = [0]

    # Optional ragged hypotheses
    hyp_rows: List[np.ndarray] = []
    hyp_offsets: List[int] = [0]

    n_groups_total = 0
    n_groups_with_cloud = 0

    for file_idx, clusters_file in enumerate(files, 1):
        print(f"\n=== [{file_idx}/{len(files)}] Loading: {clusters_file} ===")

        loader = MatchedClustersLoader(
            clusters_file,
            tdc_to_tpc=not args.no_tdc_to_tpc,
            tdc_to_tpc_factor=args.tdc_factor,
            verbose=args.verbose,
        )

        result = loader.load(build_matches=True, require_nonnegative_match_id=not args.keep_negative)
        groups_dict: Dict[Tuple[int, int], MatchedGroup] = result.get("matches_by_id", {})
        if not groups_dict:
            print("No matched groups found in file; skipping.")
            continue

        groups = sorted(groups_dict.values(), key=lambda g: (g.event_id, g.match_id))
        print(f"Loaded {len(groups)} matched groups")
        n_groups_total += len(groups)

        for i, group in enumerate(groups, 1):
            if args.skip_event_id is not None and group.event_id == int(args.skip_event_id):
                continue

            ev_id = int(group.event_id)
            mid = int(group.match_id)

            if args.verbose:
                print(f"[{i}/{len(groups)}] Event {ev_id}, match_id {mid}")

            # truth + collection ADC
            ev_ids.append(ev_id)
            match_ids.append(mid)
            col_adcs.append(_collection_adc_from_xplane(group))
            truth_xyz_list.append(_truth_xyz(group))
            truth_mom_list.append(_truth_mom_xyz(group))

            # (Optional) compute backtracked wire IDs for debugging
            if args.verbose and (group.true_x is not None and group.true_y is not None and group.true_z is not None):
                try:
                    U_id, V_id, X_id = xyz_to_UVX_wires_lut(
                        group.true_x, group.true_y, group.true_z,
                        u_y_sorted=U_y_sorted, u_u_sorted=U_u_sorted,
                        v_y_sorted=V_y_sorted, v_v_sorted=V_v_sorted
                    )
                    print(f"  Backtracked ids: X:{X_id} U:{U_id} V:{V_id}")
                except Exception as e:
                    print(f"  [WARN] backtracking failed: {e}")

            # 3D reconstruction
            cloud_np = np.zeros((0, 4), dtype=np.float32)
            hyp_np = np.zeros((0, 3), dtype=np.float32)

            try:
                cand = reco_3d.generate_candidates(
                    group,
                    use_peak_tick=bool(args.use_peak_tick),
                    expand_dt=int(args.expand_dt),
                )
                xyz_hyp = reco_3d.process_candidates_to_xyz(cand)  # list-like of candidate tuples

                if args.save_all_hypotheses:
                    # xyz_hyp elements look like (..., x, y, z, ...) per your usage
                    hyp_np = np.array([(c[1], c[2], c[3]) for c in xyz_hyp], dtype=np.float32)

                Y_U, Y_V, Y_X = reco_3d.build_measurements_from_candidates(cand)

                # Keep OMP call for parity / debugging (even if you use cloud_dense for the saved cloud)
                omp_res = reco_3d.omp_select_min_support(
                    xyz_hyp,
                    Y_U, Y_V, Y_X,
                    max_iters=int(args.omp_iters),
                    stop_l1=float(args.omp_stop_l1),
                    max_reuse_per_bin=int(args.omp_max_reuse),
                )

                cloud_res = reco_3d.reconstruct_cloud_dense(
                    xyz_hyp,
                    Y_U, Y_V, Y_X,
                    max_passes=5,
                    prefer_triples=1.2,
                    softmax_temp=1.0,
                )

                # cloud_res.selected is iterable of (amp, cand_sel) where cand_sel has (.., x, y, z, ..)
                cloud_np = np.array(
                    [(float(cand_sel[1]), float(cand_sel[2]), float(cand_sel[3]), float(amp))
                     for (amp, cand_sel) in cloud_res.selected],
                    dtype=np.float32,
                )

                if cloud_np.shape[0] > 0:
                    n_groups_with_cloud += 1

                if args.verbose:
                    sel_n = getattr(omp_res, "selected", [])
                    res_l1 = getattr(omp_res, "residual_norm", np.nan)
                    print(f"  3D: hypotheses={len(xyz_hyp)} cloud_selected={cloud_np.shape[0]} "
                          f"omp_selected={len(sel_n)} residual_L1={float(res_l1):.3g}")

            except Exception as e:
                if args.verbose:
                    print(f"  [WARN] 3D reco failed: {e}")

            # Append ragged cloud
            cloud_rows.append(cloud_np)
            cloud_offsets.append(cloud_offsets[-1] + int(cloud_np.shape[0]))

            # Append ragged hypotheses
            if args.save_all_hypotheses:
                hyp_rows.append(hyp_np)
                hyp_offsets.append(hyp_offsets[-1] + int(hyp_np.shape[0]))

    if not ev_ids:
        raise SystemExit("ERROR: No groups processed (nothing to write).")

    # Pack arrays
    event_id_arr = np.asarray(ev_ids, dtype=np.int64)
    match_id_arr = np.asarray(match_ids, dtype=np.int64)
    collection_adc_arr = np.asarray(col_adcs, dtype=np.float64)
    truth_xyz_arr = np.stack(truth_xyz_list, axis=0).astype(np.float64, copy=False)
    truth_mom_xyz_arr = np.stack(truth_mom_list, axis=0).astype(np.float64, copy=False)

    if cloud_offsets[-1] > 0:
        cloud_data = np.concatenate(cloud_rows, axis=0).astype(np.float32, copy=False)
    else:
        cloud_data = np.zeros((0, 4), dtype=np.float32)
    cloud_offsets_arr = np.asarray(cloud_offsets, dtype=np.int64)

    out_payload: Dict[str, Any] = dict(
        event_id=event_id_arr,
        match_id=match_id_arr,
        collection_adc=collection_adc_arr,
        truth_xyz=truth_xyz_arr,
        truth_mom_xyz=truth_mom_xyz_arr,
        cloud_data=cloud_data,
        cloud_offsets=cloud_offsets_arr,
    )

    if args.save_all_hypotheses:
        if hyp_offsets[-1] > 0:
            hyp_data = np.concatenate(hyp_rows, axis=0).astype(np.float32, copy=False)
        else:
            hyp_data = np.zeros((0, 3), dtype=np.float32)
        out_payload["hyp_data"] = hyp_data
        out_payload["hyp_offsets"] = np.asarray(hyp_offsets, dtype=np.int64)

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(str(out_path), **out_payload)

    print("\n=== Summary ===")
    print(f"Input files: {len(files)}")
    print(f"Groups processed (after skips): {len(event_id_arr)}")
    print(f"Groups with non-empty cloud: {n_groups_with_cloud}")
    print(f"Total cloud points: {cloud_data.shape[0]}")
    if args.save_all_hypotheses:
        print(f"Total hypothesis points: {out_payload['hyp_data'].shape[0]}")
    print(f"Wrote: {out_path}")


if __name__ == "__main__":
    main()
