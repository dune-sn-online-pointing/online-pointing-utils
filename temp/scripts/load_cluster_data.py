#!/usr/bin/env python3
"""
Helper utilities for loading cluster image data with metadata
"""

import numpy as np
from pathlib import Path
from typing import Tuple, Dict, List


def load_cluster(npz_file: str) -> Tuple[np.ndarray, Dict]:
    """
    Load a cluster image and metadata from .npz file
    
    Args:
        npz_file: Path to .npz file
        
    Returns:
        (image, metadata_dict)
        - image: 128×16 float32 array with raw ADC values
        - metadata_dict: Dictionary with decoded metadata
    """
    data = np.load(npz_file)
    
    # Load arrays
    img = data['image']
    meta = data['metadata']
    
    # Decode metadata
    plane_map = {0: 'U', 1: 'V', 2: 'X'}
    metadata_dict = {
        'is_marley': bool(meta[0]),
        'is_main_track': bool(meta[1]),
        'true_pos': meta[2:5],  # [x, y, z] in cm
        'true_dir': meta[5:8],  # [dx, dy, dz] normalized
        'true_nu_energy': float(meta[8]),  # MeV
        'true_particle_energy': float(meta[9]),  # MeV
        'plane': plane_map[int(meta[10])],
        'plane_id': int(meta[10])
    }
    
    data.close()
    
    return img, metadata_dict


def load_dataset(data_dir: str, plane: str = None, 
                 marley_only: bool = False,
                 main_track_only: bool = False) -> Tuple[np.ndarray, np.ndarray]:
    """
    Load all clusters from a directory
    
    Args:
        data_dir: Directory containing .npz files
        plane: Filter by plane ('U', 'V', 'X'), None for all
        marley_only: Only load Marley clusters
        main_track_only: Only load main track clusters
        
    Returns:
        (images, metadata)
        - images: N×128×16 float32 array
        - metadata: N×11 float32 array (raw metadata)
    """
    data_dir = Path(data_dir)
    
    # Find files
    if plane:
        pattern = f'cluster_plane{plane}_*.npz'
    else:
        pattern = 'cluster_plane*.npz'
    
    files = sorted(data_dir.glob(pattern))
    
    images = []
    metadata = []
    
    for f in files:
        data = np.load(f)
        img = data['image']
        meta = data['metadata']
        
        # Apply filters
        if marley_only and not bool(meta[0]):
            data.close()
            continue
        if main_track_only and not bool(meta[1]):
            data.close()
            continue
        
        images.append(img)
        metadata.append(meta)
        data.close()
    
    return np.array(images), np.array(metadata)


def get_dataset_info(data_dir: str) -> Dict:
    """
    Get summary information about a dataset
    
    Args:
        data_dir: Directory containing .npz files
        
    Returns:
        Dictionary with dataset statistics
    """
    data_dir = Path(data_dir)
    files = list(data_dir.glob('cluster_plane*.npz'))
    
    if not files:
        return {'total_files': 0}
    
    # Count by plane
    plane_counts = {'U': 0, 'V': 0, 'X': 0}
    marley_count = 0
    main_track_count = 0
    total_size = 0
    
    for f in files:
        total_size += f.stat().st_size
        data = np.load(f)
        meta = data['metadata']
        
        plane_id = int(meta[10])
        plane = {0: 'U', 1: 'V', 2: 'X'}[plane_id]
        plane_counts[plane] += 1
        
        if bool(meta[0]):
            marley_count += 1
        if bool(meta[1]):
            main_track_count += 1
        
        data.close()
    
    return {
        'total_files': len(files),
        'plane_counts': plane_counts,
        'marley_count': marley_count,
        'main_track_count': main_track_count,
        'total_size_mb': total_size / 1024 / 1024,
        'avg_size_kb': total_size / len(files) / 1024
    }


if __name__ == '__main__':
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python load_cluster_data.py <data_dir> [npz_file]")
        print("\nExamples:")
        print("  # Show dataset info")
        print("  python load_cluster_data.py data/prod_es/images_es_valid_bg_tick3_ch2_min2_tot1_e0")
        print("\n  # Load and inspect specific file")
        print("  python load_cluster_data.py data/prod_es/images_es_valid_bg_tick3_ch2_min2_tot1_e0 cluster_planeU_0002.npz")
        sys.exit(1)
    
    data_dir = sys.argv[1]
    
    if len(sys.argv) == 3:
        # Load specific file
        npz_file = Path(data_dir) / sys.argv[2]
        if not npz_file.exists():
            print(f"Error: File not found: {npz_file}")
            sys.exit(1)
        
        print(f"Loading: {npz_file}")
        img, meta = load_cluster(npz_file)
        
        print(f"\n📊 Metadata:")
        for key, value in meta.items():
            if isinstance(value, np.ndarray):
                print(f"  {key}: {value}")
            else:
                print(f"  {key}: {value}")
        
        print(f"\n🖼️  Image:")
        print(f"  Shape: {img.shape}")
        print(f"  Dtype: {img.dtype}")
        print(f"  Range: [{img.min():.1f}, {img.max():.1f}] ADC")
        print(f"  Non-zero pixels: {np.count_nonzero(img)}")
    else:
        # Show dataset info
        print(f"Dataset: {data_dir}")
        info = get_dataset_info(data_dir)
        
        print(f"\n📊 Dataset Statistics:")
        print(f"  Total files: {info['total_files']}")
        print(f"  Total size: {info['total_size_mb']:.2f} MB")
        print(f"  Avg file size: {info['avg_size_kb']:.2f} KB")
        print(f"\n  Plane distribution:")
        for plane, count in info['plane_counts'].items():
            pct = 100 * count / info['total_files'] if info['total_files'] > 0 else 0
            print(f"    {plane}: {count} ({pct:.1f}%)")
        print(f"\n  Marley clusters: {info['marley_count']} ({100*info['marley_count']/info['total_files']:.1f}%)")
        print(f"  Main track clusters: {info['main_track_count']} ({100*info['main_track_count']/info['total_files']:.1f}%)")
