#!/usr/bin/env python3
"""
Diagnostic script to analyze cluster properties and find large temporal/spatial spans.
"""
import ROOT
import numpy as np

# Open the clusters file
f = ROOT.TFile.Open("/home/virgolaema/dune/online-pointing-utils/data/es_valid_tick5_ch2_min2_tot1_clusters.root")

# Get clusters directory
clusters_dir = f.GetDirectory("clusters")

def analyze_plane(plane_name):
    """Analyze clusters for a given plane."""
    tree_name = f"clusters_tree_{plane_name}"
    tree = clusters_dir.Get(tree_name)
    
    if not tree:
        print(f"Tree {tree_name} not found")
        return
    
    print(f"\n{'='*60}")
    print(f"Analyzing {plane_name} plane clusters")
    print(f"{'='*60}")
    
    # Variables for branches
    n_tps = ROOT.std.vector('int')()
    tp_time_start = ROOT.std.vector('int')()
    tp_detector_channel = ROOT.std.vector('int')()
    tp_samples_over_threshold = ROOT.std.vector('int')()
    event = ROOT.std.vector('int')()
    
    tree.SetBranchAddress("n_tps", n_tps)
    tree.SetBranchAddress("tp_time_start", tp_time_start)
    tree.SetBranchAddress("tp_detector_channel", tp_detector_channel)
    tree.SetBranchAddress("tp_samples_over_threshold", tp_samples_over_threshold)
    tree.SetBranchAddress("event", event)
    
    suspicious_clusters = []
    
    for cluster_idx in range(tree.GetEntries()):
        tree.GetEntry(cluster_idx)
        
        n = n_tps[0] if n_tps.size() > 0 else 0
        if n < 2:
            continue
        
        # Get time and channel arrays
        times = [tp_time_start[i] for i in range(n)]
        channels = [tp_detector_channel[i] for i in range(n)]
        tov = [tp_samples_over_threshold[i] for i in range(n)]
        
        # Calculate time span
        time_min = min(times)
        time_max = max(times)
        time_span = time_max - time_min
        
        # Calculate channel span
        ch_min = min(channels)
        ch_max = max(channels)
        ch_span = ch_max - ch_min
        
        # Check for suspicious clusters (large temporal span)
        if time_span > 20:  # More than 4x the clustering parameter (5 ticks)
            suspicious_clusters.append((cluster_idx, n, time_span, ch_span, times, channels))
    
    print(f"\nFound {len(suspicious_clusters)} clusters with time_span > 20 ticks")
    print(f"Total clusters: {tree.GetEntries()}")
    
    # Show details of most suspicious clusters
    suspicious_clusters.sort(key=lambda x: x[2], reverse=True)
    
    for i, (cluster_idx, n, time_span, ch_span, times, channels) in enumerate(suspicious_clusters[:10]):
        print(f"\nCluster {cluster_idx} ({plane_name}): n_tps={n}, time_span={time_span}, ch_span={ch_span}")
        
        # Sort TPs by time and show bridging
        sorted_indices = sorted(range(len(times)), key=lambda i: times[i])
        
        print(f"  TPs sorted by time:")
        for j, idx in enumerate(sorted_indices):
            t = times[idx]
            ch = channels[idx]
            if j > 0:
                prev_idx = sorted_indices[j-1]
                dt = t - times[prev_idx]
                dch = abs(ch - channels[prev_idx])
                print(f"    TP{j}: t={t:6d}, ch={ch:5d} | Δt={dt:4d}, Δch={dch:3d}")
            else:
                print(f"    TP{j}: t={t:6d}, ch={ch:5d}")

# Analyze all planes
for plane in ["U", "V", "X"]:
    analyze_plane(plane)

f.Close()
print(f"\n{'='*60}")
print("Analysis complete")
print(f"{'='*60}")
