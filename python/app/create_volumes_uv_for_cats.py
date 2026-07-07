#!/usr/bin/env python3
"""
Produce U/V-plane volume images for burst-sample cats that already have X.

The original burst-sample volume production only wrote plane X. This driver
adds U/ and V/ subfolders next to the existing X/ without touching the X
files (create_volumes.py --override would delete and regenerate them, which
we do not want for a data product already used by scenario campaigns).

Volumes are centered on main-track clusters per plane; cross-plane matching
of the same interaction is possible offline via metadata keys
(event, main_cluster_match_id) written by process_cluster_file.

Usage:
    python3 python/app/create_volumes_uv_for_cats.py \
        --samples-base /eos/project-e/ep-nu/evilla/sn-online-pointing/sn-burst-samples \
        --conditions tick3_ch2_min2_tot3_e3p0 \
        --cats cat000400,cat000401
    # or --cat-range 400 621
"""

import os
import sys
import glob
import argparse
from pathlib import Path

sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from create_volumes import process_cluster_file  # noqa: E402


def parse_args():
    ap = argparse.ArgumentParser(description='Create U/V volume images for cats with existing X volumes')
    ap.add_argument('--samples-base', required=True)
    ap.add_argument('--conditions', default='tick3_ch2_min2_tot3_e3p0')
    ap.add_argument('--cats', default=None, help='Comma-separated cat names')
    ap.add_argument('--cat-range', nargs=2, type=int, default=None,
                    metavar=('LO', 'HI'), help='Inclusive cat number range')
    ap.add_argument('--planes', default='U,V')
    ap.add_argument('--verbose', '-v', action='store_true')
    return ap.parse_args()


def main():
    args = parse_args()
    planes = [p.strip() for p in args.planes.split(',') if p.strip()]

    if args.cats:
        cats = [c.strip() for c in args.cats.split(',') if c.strip()]
    elif args.cat_range:
        cats = [f'cat{i:06d}' for i in range(args.cat_range[0], args.cat_range[1] + 1)]
    else:
        print('ERROR: provide --cats or --cat-range', file=sys.stderr)
        return 1

    total_volumes = 0
    for cat in cats:
        cat_dir = os.path.join(args.samples_base, cat)
        matched_dir = os.path.join(cat_dir, f'{cat}_matched_clusters_{args.conditions}')
        volume_dir = os.path.join(cat_dir, f'{cat}_volume_images_{args.conditions}')
        if not os.path.isdir(matched_dir):
            print(f'[{cat}] SKIP: no matched clusters dir {matched_dir}')
            continue
        if not os.path.isdir(os.path.join(volume_dir, 'X')):
            print(f'[{cat}] SKIP: no existing X volumes at {volume_dir}/X')
            continue

        matched_files = sorted(glob.glob(os.path.join(matched_dir, '*_matched.root')))
        print(f'[{cat}] {len(matched_files)} matched files -> planes {planes}', flush=True)

        for mf in matched_files:
            base_name = Path(mf).stem.replace('_matched', '')
            # skip file if all requested plane outputs already exist
            if all(os.path.isfile(os.path.join(volume_dir, p, f'{base_name}_plane{p}.npz'))
                   for p in planes):
                continue
            try:
                n = process_cluster_file(mf, volume_dir, planes=planes,
                                         verbose=args.verbose)
                total_volumes += n
            except Exception as e:
                print(f'[{cat}] WARNING: {Path(mf).name} failed: {e}', flush=True)

        print(f'[{cat}] done (cumulative volumes: {total_volumes})', flush=True)

    print(f'ALL DONE: {total_volumes} volumes created across {len(cats)} cats')
    return 0


if __name__ == '__main__':
    sys.exit(main())
