#!/usr/bin/env python3
"""
Verify that U and V cluster images are correctly oriented after wire-direction flipping.

For every U/V cluster in the NPZ files that has a valid reco position, this script:

  1. Re-runs check_uv_wire_direction_from_reco(reco_x, reco_y, reco_z).
  2. Asserts the direction is determinable (not None) — tests that the reco position
     is valid and inside the detector.
  3. Asserts u_inc == v_inc — the key geometry invariant: both U and V wire indices
     change in the SAME direction with y at any given (x, z).  The direction is set
     by the APA face (True for even APA / face 0, False for odd APA / face 1).
     If they disagree, the LUT is returning inconsistent results.
  4. Counts what fraction of clusters needed a channel-axis flip and reports a
     breakdown per (apa_id, x_sign) configuration.

Usage:
    python verify_wire_directions.py --npz-dir /path/to/cluster_images/
    python verify_wire_directions.py --json json/es_production.json
    python verify_wire_directions.py --json json/es_production.json --max-files 5 -v
"""

import sys
import argparse
import json
import numpy as np
from pathlib import Path
from collections import defaultdict

# ── path setup ────────────────────────────────────────────────────────────────
_HERE = Path(__file__).parent
sys.path.insert(0, str(_HERE.parent / 'lib'))
sys.path.insert(0, str(_HERE.parent / 'reco'))
sys.path.insert(0, str(_HERE.parent / 'app'))

from utils import get_matched_clusters_folder
from generate_cluster_arrays import (
    check_uv_wire_direction_from_reco,
    get_images_folder,
)

# ── metadata column indices ───────────────────────────────────────────────────
COL_IS_MARLEY   = 1
COL_PLANE_ID    = 12
COL_MATCH_ID    = 13
COL_TRUE_Y      = 5
COL_RECO_X      = 18
COL_RECO_Y      = 19
COL_RECO_Z      = 20
COL_APA_ID      = 21

PLANE_NAMES = {0: 'U', 1: 'V', 2: 'X'}


# ── per-file checker ──────────────────────────────────────────────────────────

def check_npz_file(npz_path: Path, verbose: bool = False) -> dict:
    """
    Run wire-direction checks on every U/V cluster in one NPZ file.

    Returns a dict with:
        total, ok, fail_none, fail_same_dir,
        n_flipped_u, n_flipped_v,
        config_counts: {(apa_id, x_sign_implicit): {'u_inc': int, 'u_dec': int, 'v_inc': int, 'v_dec': int}}
        reco_y_errors: list of |reco_y - true_y| for Marley clusters (cm)
    """
    stats = dict(total=0, ok=0, fail_none=0, fail_same_dir=0,
                 n_flipped_u=0, n_flipped_v=0,
                 config_counts=defaultdict(lambda: defaultdict(int)),
                 reco_y_errors=[])

    try:
        data = np.load(npz_path)
        metadata = data['metadata']   # shape (N, 22+)
    except Exception as e:
        print(f"  [SKIP] Cannot load {npz_path.name}: {e}")
        return stats

    plane_name = npz_path.stem.split('_plane')[-1]   # 'U' or 'V'
    if plane_name not in ('U', 'V'):
        return stats

    for row in metadata:
        plane_id = int(row[COL_PLANE_ID])
        if PLANE_NAMES.get(plane_id) != plane_name:
            continue

        reco_x = float(row[COL_RECO_X])
        reco_y = float(row[COL_RECO_Y])
        reco_z = float(row[COL_RECO_Z])

        if np.isnan(reco_x):   # unmatched cluster — skip
            continue

        stats['total'] += 1

        # ── Check 1 & 2: direction must be determinable and opposite ──────────
        u_inc, v_inc = check_uv_wire_direction_from_reco(reco_x, reco_y, reco_z)

        if u_inc is None or v_inc is None:
            stats['fail_none'] += 1
            if verbose:
                print(f"  [FAIL-NONE] {plane_name} reco=({reco_x:.1f},{reco_y:.1f},{reco_z:.1f})"
                      f" → u_inc={u_inc} v_inc={v_inc}")
            continue

        if u_inc != v_inc:
            stats['fail_same_dir'] += 1
            if verbose:
                print(f"  [FAIL-DIR]  {plane_name} reco=({reco_x:.1f},{reco_y:.1f},{reco_z:.1f})"
                      f" → u_inc={u_inc} v_inc={v_inc}  (must be same direction!)")
            continue

        stats['ok'] += 1

        # ── Flip accounting ───────────────────────────────────────────────────
        # Canonical after flip: U increases with y (u_inc=True), V decreases (v_inc=False).
        # u_inc==v_inc always (same face); flip is needed when the raw direction
        # doesn't match the canonical: flip U if u_inc=False, flip V if v_inc=True.
        if plane_name == 'U' and not u_inc:
            stats['n_flipped_u'] += 1
        if plane_name == 'V' and v_inc:
            stats['n_flipped_v'] += 1

        # ── Config breakdown ─────────────────────────────────────────────────
        apa_id = int(row[COL_APA_ID])
        cfg_key = apa_id
        stats['config_counts'][cfg_key]['u_inc' if u_inc else 'u_dec'] += 1
        stats['config_counts'][cfg_key]['v_inc' if v_inc else 'v_dec'] += 1

        # ── Reco accuracy for signal clusters ─────────────────────────────────
        if int(row[COL_IS_MARLEY]) == 1:
            true_y = float(row[COL_TRUE_Y])
            stats['reco_y_errors'].append(abs(reco_y - true_y))

    return stats


# ── summary printer ──────────────────────────────────────────────────────────

def print_summary(all_stats: dict, plane: str):
    s = all_stats
    total = s['total']
    if total == 0:
        print(f"  {plane}: no matched clusters found")
        return

    ok_pct        = 100 * s['ok'] / total
    fail_none_pct = 100 * s['fail_none'] / total
    fail_dir_pct  = 100 * s['fail_same_dir'] / total

    print(f"\n{'─'*60}")
    print(f"  Plane {plane}  —  {total} matched clusters checked")
    print(f"  PASS  (direction OK, opposite U/V): {s['ok']:6d}  ({ok_pct:.1f}%)")
    print(f"  FAIL  (direction undetermined):     {s['fail_none']:6d}  ({fail_none_pct:.1f}%)")
    print(f"  FAIL  (U and V opposite direction):  {s['fail_same_dir']:6d}  ({fail_dir_pct:.1f}%)")

    flip_key = 'n_flipped_u' if plane == 'U' else 'n_flipped_v'
    if s['ok'] > 0:
        flip_pct = 100 * s[flip_key] / s['ok']
        print(f"  Channel-axis flips applied:         {s[flip_key]:6d}  ({flip_pct:.1f}% of passing)")

    if s['reco_y_errors']:
        errs = np.array(s['reco_y_errors'])
        print(f"\n  Reco-y accuracy (Marley clusters only, N={len(errs)}):")
        print(f"    |reco_y - true_y|  mean={errs.mean():.1f} cm  "
              f"median={np.median(errs):.1f} cm  p95={np.percentile(errs,95):.1f} cm")

    if s['config_counts']:
        print(f"\n  Per-APA direction breakdown (U↑ / U↓ / V↑ / V↓):")
        for apa_id in sorted(s['config_counts']):
            c = s['config_counts'][apa_id]
            print(f"    apa={apa_id}:  U↑={c.get('u_inc',0):4d}  U↓={c.get('u_dec',0):4d}"
                  f"  V↑={c.get('v_inc',0):4d}  V↓={c.get('v_dec',0):4d}")

    # Overall verdict
    if s['fail_none'] == 0 and s['fail_same_dir'] == 0:
        print(f"\n  ✓ ALL {total} clusters PASSED for plane {plane}")
    else:
        print(f"\n  ✗ {s['fail_none'] + s['fail_same_dir']} FAILURES in plane {plane}")


# ── main ─────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description='Verify wire-direction orientation of cluster images')
    parser.add_argument('--npz-dir', help='Directory containing U/ and V/ NPZ subdirectories')
    parser.add_argument('--json', '-j', help='JSON config — auto-detects images folder')
    parser.add_argument('--max-files', type=int, default=None,
                        help='Process at most N NPZ files per plane')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Print each failing cluster')
    args = parser.parse_args()

    # ── Resolve NPZ directory ─────────────────────────────────────────────────
    if args.npz_dir:
        npz_root = Path(args.npz_dir)
    elif args.json:
        with open(args.json) as f:
            cfg = json.load(f)
        images_folder = get_images_folder(cfg)
        npz_root = Path(images_folder)
        print(f"Auto-detected images folder: {npz_root}")
    else:
        parser.error("Provide --npz-dir or --json")

    if not npz_root.exists():
        print(f"ERROR: directory not found: {npz_root}")
        sys.exit(1)

    # ── Run checks per plane ──────────────────────────────────────────────────
    total_failures = 0

    for plane in ('U', 'V'):
        plane_dir = npz_root / plane
        if not plane_dir.exists():
            print(f"  Plane {plane}: directory {plane_dir} not found — skipping")
            continue

        npz_files = sorted(plane_dir.glob(f'*_plane{plane}.npz'))
        if args.max_files:
            npz_files = npz_files[:args.max_files]

        if not npz_files:
            print(f"  Plane {plane}: no NPZ files found in {plane_dir}")
            continue

        print(f"\nChecking plane {plane}: {len(npz_files)} file(s) in {plane_dir}")

        combined = dict(total=0, ok=0, fail_none=0, fail_same_dir=0,
                        n_flipped_u=0, n_flipped_v=0,
                        config_counts=defaultdict(lambda: defaultdict(int)),
                        reco_y_errors=[])

        for npz_path in npz_files:
            if args.verbose:
                print(f"  {npz_path.name}")
            s = check_npz_file(npz_path, verbose=args.verbose)
            combined['total']        += s['total']
            combined['ok']           += s['ok']
            combined['fail_none']    += s['fail_none']
            combined['fail_same_dir']+= s['fail_same_dir']
            combined['n_flipped_u']  += s['n_flipped_u']
            combined['n_flipped_v']  += s['n_flipped_v']
            combined['reco_y_errors'].extend(s['reco_y_errors'])
            for apa_id, counts in s['config_counts'].items():
                for k, v in counts.items():
                    combined['config_counts'][apa_id][k] += v

        print_summary(combined, plane)
        total_failures += combined['fail_none'] + combined['fail_same_dir']

    print(f"\n{'='*60}")
    if total_failures == 0:
        print("OVERALL: ALL checks passed.")
        sys.exit(0)
    else:
        print(f"OVERALL: {total_failures} failure(s) detected.")
        sys.exit(1)


if __name__ == '__main__':
    main()
