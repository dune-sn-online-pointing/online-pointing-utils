#!/usr/bin/env python3
"""
display_volumes.py - Display volume images for inspection

Utility script to visualize 1m x 1m volume images created for channel tagging.
"""

import numpy as np
import matplotlib.pyplot as plt
import argparse
import sys
from pathlib import Path

def load_volume(npz_file):
    """Load volume image and metadata from npz file."""
    data = np.load(npz_file, allow_pickle=True)
    image = data['image']
    metadata = data['metadata'][0]
    return image, metadata


def display_volume(npz_file, save_path=None):
    """Display a single volume image with metadata."""
    image, metadata = load_volume(npz_file)
    
    fig, ax = plt.subplots(1, 1, figsize=(12, 10))
    
    # Display image
    im = ax.imshow(image.T, aspect='auto', origin='lower', cmap='viridis', interpolation='nearest')
    
    # Add labels
    ax.set_xlabel('Channel', fontsize=12)
    ax.set_ylabel('Time (TPC ticks)', fontsize=12)
    
    # Create title with metadata
    interaction_type = metadata['interaction_type']
    particle_energy = metadata.get('particle_energy', metadata.get('neutrino_energy', -1.0))
    
    # Format energy display
    if particle_energy < 0 or interaction_type == "Background":
        energy_str = "Background"
    else:
        energy_str = f"E_e = {particle_energy:.1f} MeV"
    
    title = (f"Volume Image - Event {metadata['event']}, Plane {metadata['plane']}\n"
             f"Interaction: {interaction_type}, {energy_str}\n"
             f"Main track |p| = {metadata['main_track_momentum']:.2f} MeV/c, "
             f"Clusters: {metadata['n_clusters_in_volume']} "
             f"({metadata['n_marley_clusters']} marley, {metadata['n_non_marley_clusters']} non-marley)")
    
    ax.set_title(title, fontsize=10)
    
    # Add colorbar
    cbar = plt.colorbar(im, ax=ax)
    cbar.set_label('ADC', fontsize=12)
    
    # Add metadata text box
    textstr = '\n'.join([
        f"Center: ch={metadata['center_channel']:.1f}, t={metadata['center_time_tpc']:.1f}",
        f"Volume: {metadata['volume_size_cm']} cm × {metadata['volume_size_cm']} cm",
        f"Image shape: {metadata['image_shape']}",
        f"Main track momentum:",
        f"  px = {metadata['main_track_momentum_x']:.2f}",
        f"  py = {metadata['main_track_momentum_y']:.2f}",
        f"  pz = {metadata['main_track_momentum_z']:.2f}"
    ])
    
    props = dict(boxstyle='round', facecolor='wheat', alpha=0.8)
    ax.text(0.02, 0.98, textstr, transform=ax.transAxes, fontsize=9,
            verticalalignment='top', bbox=props)
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Saved to: {save_path}")
    else:
        plt.show()
    
    plt.close()


def display_multiple_volumes(npz_files, n_cols=3, save_path=None):
    """Display multiple volume images in a grid."""
    n_files = len(npz_files)
    n_rows = (n_files + n_cols - 1) // n_cols
    
    fig, axes = plt.subplots(n_rows, n_cols, figsize=(6*n_cols, 5*n_rows))
    
    if n_rows == 1:
        axes = axes.reshape(1, -1)
    
    for idx, npz_file in enumerate(npz_files):
        row = idx // n_cols
        col = idx % n_cols
        ax = axes[row, col]
        
        try:
            image, metadata = load_volume(npz_file)
            
            im = ax.imshow(image.T, aspect='auto', origin='lower', cmap='viridis', interpolation='nearest')
            
            title = (f"Event {metadata['event']}, {metadata['interaction_type']}\n"
                     f"E_ν={metadata['neutrino_energy']:.1f} MeV, "
                     f"Clusters: {metadata['n_clusters_in_volume']} "
                     f"({metadata['n_marley_clusters']}M)")
            
            ax.set_title(title, fontsize=8)
            ax.set_xlabel('Channel', fontsize=8)
            ax.set_ylabel('Time', fontsize=8)
            
            # Add small colorbar
            cbar = plt.colorbar(im, ax=ax)
            cbar.ax.tick_params(labelsize=6)
            
        except Exception as e:
            ax.text(0.5, 0.5, f'Error loading\n{Path(npz_file).name}',
                   ha='center', va='center', transform=ax.transAxes)
            ax.set_xticks([])
            ax.set_yticks([])
    
    # Hide empty subplots
    for idx in range(n_files, n_rows * n_cols):
        row = idx // n_cols
        col = idx % n_cols
        axes[row, col].axis('off')
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Saved grid to: {save_path}")
    else:
        plt.show()
    
    plt.close()


def print_volume_info(npz_file):
    """Print detailed information about a volume."""
    image, metadata = load_volume(npz_file)
    
    # Format energy display
    particle_energy = metadata.get('particle_energy', metadata.get('neutrino_energy', -1.0))
    interaction_type = metadata['interaction_type']
    if particle_energy < 0 or interaction_type == "Background":
        energy_str = "Background"
    else:
        energy_str = f"{particle_energy:.2f} MeV"
    
    print(f"\nVolume: {Path(npz_file).name}")
    print("=" * 60)
    print(f"Event:              {metadata['event']}")
    print(f"Plane:              {metadata['plane']}")
    print(f"Interaction:        {interaction_type}")
    print(f"Particle energy:    {energy_str}")
    print(f"Main track |p|:     {metadata['main_track_momentum']:.2f} MeV/c")
    print(f"  px:               {metadata['main_track_momentum_x']:.2f}")
    print(f"  py:               {metadata['main_track_momentum_y']:.2f}")
    print(f"  pz:               {metadata['main_track_momentum_z']:.2f}")
    print(f"Clusters in volume: {metadata['n_clusters_in_volume']}")
    print(f"  Marley:           {metadata['n_marley_clusters']}")
    print(f"  Non-marley:       {metadata['n_non_marley_clusters']}")
    print(f"Volume size:        {metadata['volume_size_cm']} cm × {metadata['volume_size_cm']} cm")
    print(f"Image shape:        {metadata['image_shape']}")
    print(f"Center position:    ch={metadata['center_channel']:.1f}, t={metadata['center_time_tpc']:.1f}")
    print(f"Image stats:")
    print(f"  Min:              {image.min():.2f}")
    print(f"  Max:              {image.max():.2f}")
    print(f"  Mean:             {image.mean():.2f}")
    print(f"  Non-zero pixels:  {np.count_nonzero(image)}")
    print("=" * 60)


def main():
    parser = argparse.ArgumentParser(description='Display volume images for channel tagging')
    parser.add_argument('volumes', nargs='+', help='NPZ volume file(s) to display')
    parser.add_argument('--grid', action='store_true', help='Display multiple volumes in a grid')
    parser.add_argument('--info', action='store_true', help='Print detailed information instead of displaying')
    parser.add_argument('--save', type=str, help='Save figure to file instead of displaying')
    parser.add_argument('--cols', type=int, default=3, help='Number of columns for grid display')
    
    args = parser.parse_args()
    
    # Expand glob patterns
    volume_files = []
    for pattern in args.volumes:
        matches = list(Path('.').glob(pattern))
        if matches:
            volume_files.extend(matches)
        elif Path(pattern).exists():
            volume_files.append(Path(pattern))
        else:
            print(f"Warning: No files found matching '{pattern}'")
    
    if len(volume_files) == 0:
        print("Error: No volume files found")
        return 1
    
    # Sort files
    volume_files = sorted(volume_files)
    
    print(f"Found {len(volume_files)} volume file(s)")
    
    if args.info:
        # Print information for all files
        for vol_file in volume_files:
            print_volume_info(vol_file)
    elif args.grid or len(volume_files) > 1:
        # Display grid of multiple volumes
        display_multiple_volumes(volume_files, n_cols=args.cols, save_path=args.save)
    else:
        # Display single volume
        display_volume(volume_files[0], save_path=args.save)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
