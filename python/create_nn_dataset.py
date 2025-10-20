#!/usr/bin/env python3
"""
Create Neural Network Dataset from DUNE Clusters
Generates 32x32 pixel images from cluster ROOT files for neural network training.
"""

import numpy as np
import matplotlib.pyplot as plt
import sys
import json
import argparse
from pathlib import Path
import h5py

sys.path.append(str(Path(__file__).parent))
from image_creator import create_images
from utils import create_channel_map_array
try:
    import ROOT
    HAS_ROOT = True
except ImportError:
    HAS_ROOT = False
    print("Warning: ROOT not available")

def create_dataset_32x32(root_file, channel_map, image_size=32, min_tps=5, max_clusters=None, 
                         only_collection=True, verbose=False):
    """
    Create 32x32 pixel dataset from ROOT file with clusters
    
    Args:
        root_file: Path to ROOT file containing clusters
        channel_map: Channel map array from utils.create_channel_map_array()
        image_size: Size of images (default: 32)
        min_tps: Minimum number of TPs per cluster (default: 5)
        max_clusters: Maximum clusters to process (default: None = all)
        only_collection: If True, use only collection plane (default: True)
        verbose: Print progress
    
    Returns:
        dict with 'images' and 'labels' keys
    """
    # Read clusters from ROOT file
    if verbose:
        print(f"\nReading clusters from: {root_file}")
    
    # Determine which plane to read
    plane = 'X' if only_collection else None  # None means read all planes
    if plane:
        clusters = read_clusters_from_root(root_file, min_tps=min_tps, max_clusters=max_clusters, 
                                          plane=plane, verbose=verbose)
    else:
        # Read from all three trees
        clusters = read_clusters_from_root(root_file, min_tps=min_tps, max_clusters=max_clusters, 
                                          plane='X', verbose=verbose)  # Start with X for now
    
    if len(clusters) == 0:
        raise ValueError("No clusters loaded from ROOT file")
    
    # Create images for each cluster
    all_images = []
    for i, clust in enumerate(clusters):
        img_u, img_v, img_x = create_images(
            tps_to_draw=clust['tps'],
            channel_map=channel_map,
            make_fixed_size=True,
            width=image_size,
            height=image_size,
            x_margin=2,
            y_margin=2,
            only_collection=only_collection,
            verbose=False
        )
        
        # Get the plane image
        if only_collection:
            if img_x[0, 0] != -1:  # Check if image was created successfully
                # Normalize to [0, 1] range
                if img_x.max() > 0:
                    img_x = img_x / img_x.max()
                all_images.append(img_x)
            elif verbose:
                print(f"Warning: Failed to create image for cluster {i}")
        else:
            # For multi-plane, stack them (shape: height x width x n_planes)
            planes = []
            if img_u[0, 0] != -1:
                if img_u.max() > 0:
                    img_u = img_u / img_u.max()
                planes.append(img_u)
            if img_v[0, 0] != -1:
                if img_v.max() > 0:
                    img_v = img_v / img_v.max()
                planes.append(img_v)
            if img_x[0, 0] != -1:
                if img_x.max() > 0:
                    img_x = img_x / img_x.max()
                planes.append(img_x)
            
            if len(planes) == 3:
                all_images.append(np.stack(planes, axis=-1))
            elif verbose:
                print(f"Warning: Cluster {i} has only {len(planes)} planes")
        
        if verbose and (i+1) % 100 == 0:
            print(f"Created images for {i+1}/{len(clusters)} clusters...")
    
    # Convert to array
    n_planes = 1 if only_collection else 3
    dataset = np.zeros((len(all_images), image_size, image_size, n_planes), dtype=np.float32)
    for i, img in enumerate(all_images):
        if img.ndim == 2:
            dataset[i, :, :, 0] = img
        else:
            dataset[i, :, :, :] = img
    
    # Extract labels
    labels = extract_labels(clusters[:len(all_images)])
    
    if verbose:
        print(f"\nDataset created:")
        print(f"  Shape: {dataset.shape}")
        print(f"  Value range: [{dataset.min():.3f}, {dataset.max():.3f}]")
    
    return {'images': dataset, 'labels': labels}

def read_clusters_from_root(filename, min_tps=5, max_clusters=None, plane='X', verbose=False):
    """
    Read clusters directly from ROOT file
    
    Args:
        filename: Path to ROOT file
        min_tps: Minimum TPs per cluster
        max_clusters: Maximum clusters to read (None = all)
        plane: Which plane to read ('U', 'V', 'X' for collection)
        verbose: Print progress
    
    Returns:
        List of dictionaries with 'tps' and 'label' keys
    """
    if not HAS_ROOT:
        raise ImportError("ROOT is required to read cluster files")
    
    # Data type for TPs
    dt = np.dtype([
        ('time_start', int),
        ('time_over_threshold', int),
        ('time_peak', int),
        ('channel', int),
        ('adc_integral', int),
        ('adc_peak', int),
        ('detid', int),
        ('type', int),
        ('algorithm', int),
        ('version', int),
        ('flag', int)
    ])
    
    file = ROOT.TFile.Open(filename, "READ")
    if not file or file.IsZombie():
        raise IOError(f"Cannot open ROOT file: {filename}")
    
    clusters = []
    
    # Try to find the tree (may be in a directory)
    # For multi-plane files, prefer the specified plane tree
    tree_name_pattern = f'clusters_tree_{plane}'
    tree = None
    
    for key in file.GetListOfKeys():
        obj = file.Get(key.GetName())
        if obj.InheritsFrom("TTree"):
            if tree_name_pattern in obj.GetName():
                tree = obj
                if verbose:
                    print(f"Found tree: {tree.GetName()}")
                break
            elif tree is None:
                tree = obj  # Fallback to first tree found
        elif obj.InheritsFrom("TDirectoryFile"):
            # Look inside the directory
            if verbose:
                print(f"Searching in directory: {key.GetName()}")
            for subkey in obj.GetListOfKeys():
                subobj = obj.Get(subkey.GetName())
                if subobj.InheritsFrom("TTree"):
                    if tree_name_pattern in subobj.GetName():
                        tree = subobj
                        if verbose:
                            print(f"Found tree in directory: {tree.GetName()}")
                        break
                    elif tree is None:
                        tree = subobj  # Fallback to first tree found
            if tree and tree_name_pattern in tree.GetName():
                break
    
    if tree is None:
        raise ValueError("No TTree found in ROOT file")
    
    # Check required branches
    required_branches = ['tp_time_start', 'tp_adc_integral', 'tp_adc_peak']
    for branch in required_branches:
        if not tree.GetBranch(branch):
            raise ValueError(f"Required branch '{branch}' not found in tree")
    
    n_entries = tree.GetEntries()
    if verbose:
        print(f"Reading {n_entries} clusters from tree...")
    
    for i in range(n_entries):
        if max_clusters and i >= max_clusters:
            break
            
        tree.GetEntry(i)
        
        # Get number of TPs in this cluster
        try:
            n_tps = tree.n_tps if hasattr(tree, 'n_tps') else len(tree.tp_time_start)
        except:
            n_tps = 0
        
        if n_tps < min_tps:
            continue
        
        # Create TP array
        tps = np.zeros(n_tps, dtype=dt)
        
        try:
            for j in range(n_tps):
                tps[j]['time_start'] = tree.tp_time_start[j]
                tps[j]['time_over_threshold'] = tree.tp_samples_over_threshold[j]
                
                # Calculate time_peak
                time_peak = tree.tp_time_start[j] + tree.tp_samples_to_peak[j]
                tps[j]['time_peak'] = time_peak
                
                tps[j]['channel'] = tree.tp_detector_channel[j]
                tps[j]['adc_integral'] = tree.tp_adc_integral[j]
                tps[j]['adc_peak'] = tree.tp_adc_peak[j]
                tps[j]['detid'] = tree.tp_detector[j] if hasattr(tree, 'tp_detector') else 0
                tps[j]['type'] = 0
                tps[j]['algorithm'] = 0
                tps[j]['version'] = 0
                tps[j]['flag'] = 0
                
        except Exception as e:
            if verbose:
                print(f"Warning: Error reading cluster {i}: {e}")
            continue
        
        # Get label if available
        label = -1
        try:
            if hasattr(tree, 'true_label'):
                label_str = str(tree.true_label)
                # Try to convert to int
                if label_str.isdigit():
                    label = int(label_str)
                elif label_str in ['electron', 'e']:
                    label = 0
                elif label_str in ['muon', 'mu']:
                    label = 1
                elif label_str in ['other']:
                    label = 2
        except:
            pass
        
        clusters.append({'tps': tps, 'label': label})
        
        if verbose and (i+1) % 100 == 0:
            print(f"Read {i+1}/{n_entries} clusters...")
    
    file.Close()
    
    if verbose:
        print(f"Loaded {len(clusters)} clusters with >= {min_tps} TPs")
    
    return clusters

def extract_labels(clusters):
    """Extract labels from cluster dictionaries"""
    labels = np.zeros(len(clusters), dtype=np.int32)
    for i, clust in enumerate(clusters):
        labels[i] = clust['label']
    return labels

def main():
    parser = argparse.ArgumentParser(
        description='Create 32x32 neural network dataset from DUNE cluster ROOT files'
    )
    parser.add_argument('--input', '-i', type=str, required=True,
                       help='Input ROOT file with clusters')
    parser.add_argument('--output', '-o', type=str, required=True,
                       help='Output HDF5 file for dataset')
    parser.add_argument('--detector', type=str, default='APA', choices=['APA', 'CRP', '50L'],
                       help='Detector type for channel map (default: APA)')
    parser.add_argument('--min-tps', type=int, default=5,
                       help='Minimum TPs per cluster (default: 5)')
    parser.add_argument('--max-clusters', type=int, default=None,
                       help='Maximum clusters to process (for testing)')
    parser.add_argument('--only-collection', action='store_true',
                       help='Use only collection plane (X)')
    parser.add_argument('--save-samples', type=int, default=0,
                       help='Number of sample images to save as PNG (default: 0)')
    args = parser.parse_args()
    
    # Check input file exists
    if not Path(args.input).exists():
        print(f"Error: Input file not found: {args.input}")
        return 1
    
    # Load channel map
    print("Loading channel map...")
    channel_map = create_channel_map_array(args.detector)
    
    # Create dataset directly from ROOT file
    print(f"\nCreating 32x32 dataset from {args.input}...")
    result = create_dataset_32x32(
        root_file=args.input,
        channel_map=channel_map,
        image_size=32,
        min_tps=args.min_tps,
        max_clusters=args.max_clusters,
        only_collection=args.only_collection,
        verbose=True
    )
    
    dataset = result['images']
    labels = result['labels']
    
    # Save to HDF5
    print(f"\nSaving dataset to {args.output}")
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    with h5py.File(args.output, 'w') as f:
        f.create_dataset('images', data=dataset, compression='gzip')
        f.create_dataset('labels', data=labels, compression='gzip')
        f.attrs['n_clusters'] = len(dataset)
        f.attrs['image_shape'] = str(dataset.shape[1:])
        f.attrs['only_collection'] = args.only_collection
        f.attrs['min_tps'] = args.min_tps
        f.attrs['input_file'] = args.input
    
    print(f"\n{'='*60}")
    print(f"Dataset successfully created!")
    print(f"{'='*60}")
    print(f"Dataset shape: {dataset.shape}")
    print(f"Labels shape: {labels.shape}")
    print(f"Non-zero images: {np.count_nonzero(dataset.sum(axis=(1,2,3)))}/{len(dataset)}")
    print(f"Image size: 32x32 pixels")
    print(f"Output file: {args.output}")
    
    # Save sample images if requested
    if args.save_samples > 0:
        output_dir = output_path.parent / f"{output_path.stem}_samples"
        output_dir.mkdir(exist_ok=True)
        
        print(f"\nSaving {args.save_samples} sample images to {output_dir}")
        # Find non-empty images
        non_empty_mask = dataset.sum(axis=(1,2,3)) > 0
        non_empty_indices = np.where(non_empty_mask)[0]
        
        if len(non_empty_indices) > 0:
            n_samples = min(args.save_samples, len(non_empty_indices))
            indices = np.random.choice(non_empty_indices, n_samples, replace=False)
            
            for idx, i in enumerate(indices):
                img = dataset[i, :, :, 0]
                plt.figure(figsize=(6, 6))
                plt.imshow(img, cmap='viridis', origin='lower')
                plt.colorbar(label='Normalized ADC')
                plt.xlabel('Channel', fontsize=12)
                plt.ylabel('Time [ticks]', fontsize=12)
                plt.title(f'Cluster {i} (Label: {labels[i]})', fontsize=14)
                plt.tight_layout()
                plt.savefig(output_dir / f'sample_{idx:03d}_cluster_{i}.png', dpi=100)
                plt.close()
            
            print(f"Saved {n_samples} sample images")
        else:
            print("Warning: No non-empty images to save")
    
    print("\nDone!")
    return 0

if __name__ == '__main__':
    sys.exit(main())
