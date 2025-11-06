#!/usr/bin/env python3
"""
Quick viewer for volume images (.npz files)
Displays the pentagon arrays as images with channel tagging
"""

import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import argparse

def load_volume(npz_path):
    """Load volume data from .npz file"""
    data = np.load(npz_path)
    return {
        'pentagon_array': data['pentagon_array'],
        'metadata': {k: data[k].item() if data[k].shape == () else data[k] 
                    for k in data.files if k != 'pentagon_array'}
    }

def display_volume(npz_path, save_path=None):
    """Display a single volume image"""
    volume = load_volume(npz_path)
    pentagon_array = volume['pentagon_array']
    metadata = volume['metadata']
    
    # Create figure
    fig, axes = plt.subplots(1, 2, figsize=(16, 7))
    
    # Plot pentagon array (channel tagging image)
    im1 = axes[0].imshow(pentagon_array, cmap='viridis', aspect='auto', 
                         interpolation='nearest', origin='lower')
    axes[0].set_title(f'Pentagon Channel Array\nShape: {pentagon_array.shape}', 
                      fontsize=12, fontweight='bold')
    axes[0].set_xlabel('Time bin', fontsize=10)
    axes[0].set_ylabel('Channel', fontsize=10)
    plt.colorbar(im1, ax=axes[0], label='Amplitude')
    
    # Plot sum projection (to see overall activity)
    time_projection = pentagon_array.sum(axis=0)
    channel_projection = pentagon_array.sum(axis=1)
    
    axes[1].plot(time_projection, label='Time projection (sum over channels)', linewidth=2)
    axes[1].set_xlabel('Time bin', fontsize=10)
    axes[1].set_ylabel('Total amplitude', fontsize=10)
    axes[1].set_title('Activity vs Time', fontsize=12, fontweight='bold')
    axes[1].grid(True, alpha=0.3)
    axes[1].legend()
    
    # Add metadata as text
    filename = Path(npz_path).name
    fig.suptitle(f'{filename}\n' + 
                 f'Clusters: {metadata.get("num_clusters", "?")} | ' +
                 f'TPs: {metadata.get("num_tps", "?")} | ' +
                 f'Plane: {metadata.get("plane", "?")}',
                 fontsize=11, fontweight='bold')
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Saved to: {save_path}")
    else:
        plt.show()
    
    plt.close()

def display_multiple_volumes(volume_dir, max_volumes=6, pattern="*volume*.npz", save_dir=None):
    """Display a grid of multiple volume images"""
    volume_dir = Path(volume_dir)
    volume_files = sorted(volume_dir.glob(pattern))[:max_volumes]
    
    if not volume_files:
        print(f"No volume files found in {volume_dir} matching pattern {pattern}")
        return
    
    n_volumes = len(volume_files)
    cols = min(3, n_volumes)
    rows = (n_volumes + cols - 1) // cols
    
    fig, axes = plt.subplots(rows, cols, figsize=(6*cols, 4*rows))
    if n_volumes == 1:
        axes = np.array([axes])
    axes = axes.flatten()
    
    for idx, npz_path in enumerate(volume_files):
        volume = load_volume(npz_path)
        pentagon_array = volume['pentagon_array']
        metadata = volume['metadata']
        
        im = axes[idx].imshow(pentagon_array, cmap='viridis', aspect='auto',
                             interpolation='nearest', origin='lower')
        
        filename = npz_path.name
        n_clusters = metadata.get('num_clusters', '?')
        n_tps = metadata.get('num_tps', '?')
        
        axes[idx].set_title(f'{filename[:40]}...\n' +
                           f'Clusters: {n_clusters} | TPs: {n_tps}',
                           fontsize=9)
        axes[idx].set_xlabel('Time bin', fontsize=8)
        axes[idx].set_ylabel('Channel', fontsize=8)
        plt.colorbar(im, ax=axes[idx], label='Amp', fraction=0.046)
    
    # Hide empty subplots
    for idx in range(n_volumes, len(axes)):
        axes[idx].axis('off')
    
    plt.suptitle(f'Volume Images from {volume_dir.name}', 
                 fontsize=14, fontweight='bold', y=0.995)
    plt.tight_layout()
    
    if save_dir:
        save_path = Path(save_dir) / 'volume_grid.png'
        save_path.parent.mkdir(parents=True, exist_ok=True)
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"Saved grid to: {save_path}")
    else:
        plt.show()
    
    plt.close()

def main():
    parser = argparse.ArgumentParser(description='View volume images from .npz files')
    parser.add_argument('path', help='Path to .npz file or directory containing volume images')
    parser.add_argument('--max', type=int, default=6, help='Maximum number of volumes to display (default: 6)')
    parser.add_argument('--pattern', default='*volume*.npz', help='File pattern for directory mode (default: *volume*.npz)')
    parser.add_argument('--save', help='Save output to this directory instead of displaying')
    
    args = parser.parse_args()
    
    path = Path(args.path)
    
    if path.is_file():
        # Single file mode
        save_path = Path(args.save) / path.with_suffix('.png').name if args.save else None
        if save_path:
            save_path.parent.mkdir(parents=True, exist_ok=True)
        display_volume(path, save_path)
    elif path.is_dir():
        # Directory mode - show grid
        display_multiple_volumes(path, max_volumes=args.max, pattern=args.pattern, save_dir=args.save)
    else:
        print(f"Error: {path} is not a valid file or directory")
        return 1
    
    return 0

if __name__ == '__main__':
    exit(main())
