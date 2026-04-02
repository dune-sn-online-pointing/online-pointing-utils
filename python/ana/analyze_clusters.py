#!/usr/bin/env python3
"""
analyze_clusters.py - Analyze cluster properties across all cluster files.

Reads cluster ROOT files produced by make_clusters and reports:
- Cluster counts per plane
- Time span and channel span distributions
- Suspicious clusters with large temporal or spatial extent

Usage:
    python3 python/ana/analyze_clusters.py -j json/test_01.json
"""

import uproot
import numpy as np
import argparse
import json
import sys
from pathlib import Path


def sanitize(value):
    if isinstance(value, float):
        s = f"{value:.6f}"
    else:
        s = str(value)
    if '.' in s:
        parts = s.split('.')
        if len(parts[1]) > 1:
            s = f"{parts[0]}.{parts[1][0]}"
    s = s.replace('.', 'p')
    return s


def resolve_clusters_folder(config):
    """Derive the clusters folder path from the JSON config."""
    clusters_folder = config.get('clusters_folder', None)
    if clusters_folder:
        return clusters_folder

    base_folder = (config.get('signal_folder') or
                   config.get('main_folder') or
                   config.get('tpstream_folder', '.')).rstrip('/')

    prefix = config.get('products_prefix', config.get('clusters_folder_prefix', ''))

    tick_limit       = config.get('tick_limit', 0)
    channel_limit    = config.get('channel_limit', 0)
    min_tps          = config.get('min_tps_to_cluster', 0)
    tot_cut          = config.get('tot_cut', 0)
    energy_cut       = float(config.get('energy_cut', 0.0))

    conditions = (
        f"tick{sanitize(tick_limit)}"
        f"_ch{sanitize(channel_limit)}"
        f"_min{sanitize(min_tps)}"
        f"_tot{sanitize(tot_cut)}"
        f"_e{sanitize(energy_cut)}"
    )

    if prefix:
        return f"{base_folder}/{prefix}_clusters_{conditions}"
    return f"{base_folder}/clusters_{conditions}"


def analyze_plane(tree, plane, time_span_threshold=20, verbose=False):
    """
    Analyze clusters for one plane from an uproot TTree.

    Returns a dict with summary stats and a list of suspicious clusters.
    """
    try:
        arrays = tree.arrays(
            ['event', 'n_tps', 'tp_time_start', 'tp_detector_channel',
             'tp_samples_over_threshold'],
            library='np'
        )
    except Exception as e:
        print(f"  Warning: could not read plane {plane}: {e}")
        return None

    n_entries = len(arrays['event'])
    suspicious = []

    for i in range(n_entries):
        n = int(arrays['n_tps'][i]) if np.ndim(arrays['n_tps'][i]) == 0 else int(arrays['n_tps'][i][0])
        if n < 2:
            continue

        times    = arrays['tp_time_start'][i][:n]
        channels = arrays['tp_detector_channel'][i][:n]

        time_span = int(np.max(times) - np.min(times))
        ch_span   = int(np.max(channels) - np.min(channels))

        if time_span > time_span_threshold:
            suspicious.append({
                'cluster_idx': i,
                'event':       int(arrays['event'][i]),
                'n_tps':       n,
                'time_span':   time_span,
                'ch_span':     ch_span,
                'times':       times.tolist(),
                'channels':    channels.tolist(),
            })

    return {
        'plane':        plane,
        'total':        n_entries,
        'suspicious':   suspicious,
    }


def print_plane_summary(result, n_show=10):
    plane = result['plane']
    total = result['total']
    sus   = result['suspicious']

    print(f"\n{'='*60}")
    print(f"Plane {plane}: {total} clusters, {len(sus)} with time_span > 20 ticks")
    print(f"{'='*60}")

    if not sus:
        return

    sus_sorted = sorted(sus, key=lambda x: x['time_span'], reverse=True)
    for entry in sus_sorted[:n_show]:
        print(f"\n  Cluster {entry['cluster_idx']} (event {entry['event']}): "
              f"n_tps={entry['n_tps']}, time_span={entry['time_span']}, ch_span={entry['ch_span']}")

        times    = entry['times']
        channels = entry['channels']
        order    = sorted(range(len(times)), key=lambda k: times[k])

        print(f"  TPs sorted by time:")
        for j, idx in enumerate(order):
            t  = times[idx]
            ch = channels[idx]
            if j > 0:
                prev = order[j - 1]
                dt  = t  - times[prev]
                dch = abs(ch - channels[prev])
                print(f"    TP{j}: t={t:8d}, ch={ch:5d} | Δt={dt:5d}, Δch={dch:3d}")
            else:
                print(f"    TP{j}: t={t:8d}, ch={ch:5d}")


def main():
    parser = argparse.ArgumentParser(
        description='Analyze cluster properties from make_clusters output'
    )
    parser.add_argument('-j', '--json', required=True,
                        help='JSON configuration file')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Enable verbose output')
    parser.add_argument('-m', '--max-files', type=int, default=None,
                        help='Maximum number of cluster files to process')
    parser.add_argument('-s', '--skip-files', type=int, default=0,
                        help='Number of files to skip from the beginning')
    parser.add_argument('--time-threshold', type=int, default=20,
                        help='Flag clusters with time_span > N ticks (default: 20)')
    parser.add_argument('--show', type=int, default=10,
                        help='Number of suspicious clusters to print per plane per file (default: 10)')
    args = parser.parse_args()

    with open(args.json) as f:
        config = json.load(f)

    clusters_folder = resolve_clusters_folder(config)
    clusters_path   = Path(clusters_folder)

    if not clusters_path.exists():
        print(f"Error: clusters folder does not exist: {clusters_folder}")
        return 1

    cluster_files = sorted(clusters_path.glob('*.root'))
    if not cluster_files:
        print(f"No .root files found in {clusters_folder}")
        return 1

    if args.skip_files > 0:
        cluster_files = cluster_files[args.skip_files:]
    if args.max_files is not None:
        cluster_files = cluster_files[:args.max_files]

    print("="*60)
    print("Cluster Analysis")
    print("="*60)
    print(f"Folder : {clusters_folder}")
    print(f"Files  : {len(cluster_files)}")
    print(f"Time threshold: > {args.time_threshold} ticks")
    print()

    # Accumulated totals across all files
    totals      = {p: 0 for p in ('U', 'V', 'X')}
    sus_counts  = {p: 0 for p in ('U', 'V', 'X')}

    for file_idx, fpath in enumerate(cluster_files):
        if args.verbose:
            print(f"[{file_idx+1}/{len(cluster_files)}] {fpath.name}")

        try:
            root_file = uproot.open(fpath)
        except Exception as e:
            print(f"Warning: could not open {fpath.name}: {e}")
            continue

        for plane in ('U', 'V', 'X'):
            tree_key = f'clusters/clusters_tree_{plane}'
            if tree_key not in root_file:
                if args.verbose:
                    print(f"  Tree {tree_key} not found, skipping")
                continue

            result = analyze_plane(root_file[tree_key], plane,
                                   time_span_threshold=args.time_threshold,
                                   verbose=args.verbose)
            if result is None:
                continue

            totals[plane]     += result['total']
            sus_counts[plane] += len(result['suspicious'])

            if result['suspicious'] and args.verbose:
                print_plane_summary(result, n_show=args.show)

    print()
    print("="*60)
    print("SUMMARY")
    print("="*60)
    for plane in ('U', 'V', 'X'):
        t = totals[plane]
        s = sus_counts[plane]
        pct = 100.0 * s / t if t > 0 else 0.0
        print(f"  Plane {plane}: {t:6d} clusters, {s:5d} suspicious ({pct:.2f}%)")
    print("="*60)

    return 0


if __name__ == '__main__':
    sys.exit(main())
