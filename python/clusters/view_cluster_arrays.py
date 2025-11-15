#!/usr/bin/env python3
"""
View Cluster Numpy Arrays

Reads 32x32 numpy array files (.npy) generated for NN training
and displays them as images to verify correctness.

Usage:
    python view_cluster_arrays.py <cluster_folder> [--num-samples N] [--plane PLANE]
    
Examples:
    # View 10 random samples from all planes
    python view_cluster_arrays.py data/prod_es/clusters_es_valid_bg_tick3_ch2_min2_tot3_e2p0
    
    # View 20 samples from plane U only
    python view_cluster_arrays.py data/prod_es/clusters_es_valid_bg_tick3_ch2_min2_tot3_e2p0 --num-samples 20 --plane U
    
    # View specific array file
    python view_cluster_arrays.py data/prod_es/clusters_es_valid_bg_tick3_ch2_min2_tot3_e2p0/cluster_planeU_0042.npy
"""

import sys
import argparse
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt
import random


def load_and_display_array(npy_file):
    """Load and display a single numpy array"""
    array = np.load(npy_file)
    
    print(f"\nFile: {npy_file.name}")
    print(f"Shape: {array.shape}")
    print(f"Data type: {array.dtype}")
    print(f"Value range: [{array.min():.3f}, {array.max():.3f}]")
    print(f"Mean: {array.mean():.3f}, Std: {array.std():.3f}")
    
    return array


def display_samples(cluster_folder, num_samples=10, plane_filter=None):
    """Display random samples from cluster folder"""
    
    folder = Path(cluster_folder)
    
    if folder.is_file() and folder.suffix == '.npy':
        # Single file specified
        array = load_and_display_array(folder)
        
        fig, ax = plt.subplots(figsize=(6, 6))
        im = ax.imshow(array, cmap='gray', interpolation='nearest')
        ax.set_title(f'{folder.name}\nShape: {array.shape}')
        ax.set_xlabel('X pixel')
        ax.set_ylabel('Y pixel')
        plt.colorbar(im, ax=ax, label='Normalized intensity')
        plt.tight_layout()
        plt.show()
        return
    
    # Find all .npy files in folder
    if plane_filter:
        pattern = f"cluster_plane{plane_filter}_*.npy"
    else:
        pattern = "cluster_plane*.npy"
    
    npy_files = list(folder.glob(pattern))
    
    if not npy_files:
        print(f"Error: No .npy files found matching pattern '{pattern}' in {folder}")
        return
    
    print(f"Found {len(npy_files)} numpy array files")
    
    # Select random samples
    if len(npy_files) > num_samples:
        selected_files = random.sample(npy_files, num_samples)
    else:
        selected_files = npy_files
        num_samples = len(npy_files)
    
    selected_files.sort()  # Sort for consistent display
    
    # Calculate grid layout
    cols = min(5, num_samples)
    rows = (num_samples + cols - 1) // cols
    
    fig, axes = plt.subplots(rows, cols, figsize=(3*cols, 3*rows))
    if num_samples == 1:
        axes = np.array([axes])
    axes = axes.flatten()
    
    print(f"\nDisplaying {num_samples} random samples...")
    
    for idx, npy_file in enumerate(selected_files):
        try:
            array = np.load(npy_file)
            
            ax = axes[idx]
            im = ax.imshow(array, cmap='gray', interpolation='nearest')
            
            # Extract info from filename
            filename = npy_file.stem  # cluster_planeU_0042
            parts = filename.split('_')
            plane = parts[1].replace('plane', '') if len(parts) > 1 else '?'
            cluster_id = parts[2] if len(parts) > 2 else '?'
            
            ax.set_title(f'Plane {plane}, ID {cluster_id}\n{array.shape}', fontsize=9)
            ax.set_xlabel('X', fontsize=8)
            ax.set_ylabel('Y', fontsize=8)
            ax.tick_params(labelsize=7)
            
            print(f"  [{idx+1}/{num_samples}] {npy_file.name}: shape={array.shape}, "
                  f"range=[{array.min():.3f}, {array.max():.3f}]")
            
        except Exception as e:
            print(f"  Error loading {npy_file.name}: {e}")
            axes[idx].text(0.5, 0.5, 'Error', ha='center', va='center')
            axes[idx].set_title(npy_file.name, fontsize=9)
    
    # Hide unused subplots
    for idx in range(num_samples, len(axes)):
        axes[idx].axis('off')
    
    plt.tight_layout()
    plt.suptitle(f'Cluster Numpy Arrays (32x32) - {num_samples} samples', 
                 fontsize=14, y=1.02)
    plt.show()
    
    # Display statistics
    print("\n" + "="*60)
    print("Summary Statistics:")
    print("="*60)
    
    all_arrays = []
    for npy_file in npy_files[:100]:  # Sample up to 100 for stats
        try:
            arr = np.load(npy_file)
            all_arrays.append(arr)
        except:
            continue
    
    if all_arrays:
        all_data = np.array(all_arrays)
        print(f"Total files checked: {len(all_arrays)}")
        print(f"Array shape: {all_data.shape}")
        print(f"Overall value range: [{all_data.min():.3f}, {all_data.max():.3f}]")
        print(f"Overall mean: {all_data.mean():.3f}")
        print(f"Overall std: {all_data.std():.3f}")
        print(f"Sparsity (zeros): {(all_data == 0).sum() / all_data.size * 100:.1f}%")


def main():
    parser = argparse.ArgumentParser(
        description='View cluster numpy arrays generated for NN training',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # View 10 random samples
  %(prog)s data/prod_es/clusters_es_valid_bg_tick3_ch2_min2_tot3_e2p0
  
  # View 20 samples from plane U only
  %(prog)s data/prod_es/clusters_es_valid_bg_tick3_ch2_min2_tot3_e2p0 --num-samples 20 --plane U
  
  # View specific file
  %(prog)s data/prod_es/clusters_es_valid_bg_tick3_ch2_min2_tot3_e2p0/cluster_planeU_0042.npy
        """
    )
    
    parser.add_argument('cluster_folder', 
                       help='Folder containing .npy files or path to single .npy file')
    parser.add_argument('--num-samples', '-n', type=int, default=10,
                       help='Number of random samples to display (default: 10)')
    parser.add_argument('--plane', '-p', choices=['U', 'V', 'X'],
                       help='Filter by plane (U, V, or X)')
    
    args = parser.parse_args()
    
    display_samples(args.cluster_folder, args.num_samples, args.plane)


if __name__ == '__main__':
    main()
