#!/usr/bin/env python3
"""
Visualize Neural Network Dataset
Display 32x32 cluster images from HDF5 dataset files.
"""

import numpy as np
import matplotlib.pyplot as plt
import argparse
import h5py
from pathlib import Path
from matplotlib.gridspec import GridSpec

def visualize_dataset(dataset_file, n_samples=16, save_output=None, show_labels=True):
    """
    Visualize samples from the dataset in a grid
    
    Args:
        dataset_file: Path to HDF5 dataset file
        n_samples: Number of samples to display (will be arranged in a grid)
        save_output: If provided, save figure to this file instead of showing
        show_labels: Display labels on each image
    """
    print(f"Loading dataset from {dataset_file}")
    
    with h5py.File(dataset_file, 'r') as f:
        images = f['images'][:]
        labels = f['labels'][:]
        
        print(f"Dataset info:")
        print(f"  Images shape: {images.shape}")
        print(f"  Labels shape: {labels.shape}")
        print(f"  Image size: {f.attrs.get('image_shape', 'N/A')}")
        print(f"  Number of clusters: {f.attrs.get('n_clusters', len(images))}")
        print(f"  Only collection plane: {f.attrs.get('only_collection', 'N/A')}")
        print(f"  Min TPs: {f.attrs.get('min_tps', 'N/A')}")
    
    # Find non-empty images
    non_empty_mask = images.sum(axis=(1,2,3)) > 0
    non_empty_indices = np.where(non_empty_mask)[0]
    
    print(f"  Non-empty images: {len(non_empty_indices)}/{len(images)}")
    
    if len(non_empty_indices) == 0:
        print("Error: No non-empty images found!")
        return
    
    # Select random samples from non-empty images
    n_samples = min(n_samples, len(non_empty_indices))
    selected_indices = np.random.choice(non_empty_indices, n_samples, replace=False)
    
    # Calculate grid dimensions
    n_cols = int(np.ceil(np.sqrt(n_samples)))
    n_rows = int(np.ceil(n_samples / n_cols))
    
    # Create figure
    fig = plt.figure(figsize=(3*n_cols, 3*n_rows))
    fig.suptitle(f'Dataset Samples (32x32 images)\nFile: {Path(dataset_file).name}',
                 fontsize=14, fontweight='bold')
    
    # Plot each sample
    for plot_idx, img_idx in enumerate(selected_indices):
        ax = plt.subplot(n_rows, n_cols, plot_idx + 1)
        
        # Get image (use first channel if multi-channel)
        img = images[img_idx, :, :, 0]
        label = labels[img_idx]
        
        # Plot
        im = ax.imshow(img, cmap='viridis', origin='lower', aspect='auto')
        
        # Add colorbar
        plt.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
        
        # Title with label
        if show_labels:
            title = f'#{img_idx}\nLabel: {label}'
        else:
            title = f'Sample #{img_idx}'
        ax.set_title(title, fontsize=10)
        
        # Labels
        ax.set_xlabel('Channel', fontsize=8)
        ax.set_ylabel('Time [ticks]', fontsize=8)
        ax.tick_params(labelsize=7)
    
    plt.tight_layout()
    
    if save_output:
        print(f"\nSaving visualization to {save_output}")
        plt.savefig(save_output, dpi=150, bbox_inches='tight')
        print("Saved!")
    else:
        print("\nDisplaying visualization...")
        plt.show()

def plot_statistics(dataset_file, save_output=None):
    """Plot dataset statistics"""
    print(f"Analyzing dataset from {dataset_file}")
    
    with h5py.File(dataset_file, 'r') as f:
        images = f['images'][:]
        labels = f['labels'][:]
    
    # Calculate statistics
    non_empty_mask = images.sum(axis=(1,2,3)) > 0
    intensities = images[non_empty_mask].sum(axis=(1,2,3))
    max_values = images[non_empty_mask].max(axis=(1,2,3))
    
    # Create figure with multiple subplots
    fig = plt.figure(figsize=(15, 5))
    
    # Subplot 1: Label distribution
    ax1 = plt.subplot(131)
    unique_labels, counts = np.unique(labels[non_empty_mask], return_counts=True)
    ax1.bar(unique_labels, counts, color='steelblue', edgecolor='black')
    ax1.set_xlabel('Label', fontsize=12)
    ax1.set_ylabel('Count', fontsize=12)
    ax1.set_title('Label Distribution', fontsize=12, fontweight='bold')
    ax1.grid(axis='y', alpha=0.3)
    
    # Subplot 2: Intensity distribution
    ax2 = plt.subplot(132)
    ax2.hist(intensities, bins=50, color='forestgreen', edgecolor='black', alpha=0.7)
    ax2.set_xlabel('Total Intensity', fontsize=12)
    ax2.set_ylabel('Count', fontsize=12)
    ax2.set_title('Image Intensity Distribution', fontsize=12, fontweight='bold')
    ax2.grid(axis='y', alpha=0.3)
    
    # Subplot 3: Max pixel value distribution
    ax3 = plt.subplot(133)
    ax3.hist(max_values, bins=50, color='coral', edgecolor='black', alpha=0.7)
    ax3.set_xlabel('Max Pixel Value', fontsize=12)
    ax3.set_ylabel('Count', fontsize=12)
    ax3.set_title('Max Pixel Value Distribution', fontsize=12, fontweight='bold')
    ax3.grid(axis='y', alpha=0.3)
    
    plt.suptitle(f'Dataset Statistics\nFile: {Path(dataset_file).name}',
                 fontsize=14, fontweight='bold')
    plt.tight_layout()
    
    if save_output:
        print(f"\nSaving statistics to {save_output}")
        plt.savefig(save_output, dpi=150, bbox_inches='tight')
        print("Saved!")
    else:
        print("\nDisplaying statistics...")
        plt.show()

def main():
    parser = argparse.ArgumentParser(
        description='Visualize 32x32 neural network dataset'
    )
    parser.add_argument('dataset', type=str,
                       help='HDF5 dataset file to visualize')
    parser.add_argument('--n-samples', '-n', type=int, default=16,
                       help='Number of sample images to display (default: 16)')
    parser.add_argument('--output', '-o', type=str, default=None,
                       help='Save visualization to file instead of displaying')
    parser.add_argument('--stats', action='store_true',
                       help='Show dataset statistics instead of samples')
    parser.add_argument('--no-labels', action='store_true',
                       help='Do not show labels on images')
    args = parser.parse_args()
    
    if not Path(args.dataset).exists():
        print(f"Error: Dataset file not found: {args.dataset}")
        return 1
    
    if args.stats:
        plot_statistics(args.dataset, save_output=args.output)
    else:
        visualize_dataset(
            args.dataset,
            n_samples=args.n_samples,
            save_output=args.output,
            show_labels=not args.no_labels
        )
    
    return 0

if __name__ == '__main__':
    import sys
    sys.exit(main())
