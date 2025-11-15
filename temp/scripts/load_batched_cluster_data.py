#!/usr/bin/env python3
"""
Helper utilities for loading batched cluster image data with metadata
"""

import numpy as np
from pathlib import Path
from typing import Tuple, Dict, List, Optional


def load_batch(npz_file: str) -> Tuple[np.ndarray, np.ndarray]:
    """
    Load a batch of cluster images and metadata from .npz file
    
    Args:
        npz_file: Path to batch .npz file
        
    Returns:
        (images, metadata)
        - images: N×128×16 float32 array with raw ADC values
        - metadata: N×15 float32 array (raw metadata for each cluster)
    """
    data = np.load(npz_file)
    images = data['images']
    metadata = data['metadata']
    data.close()
    return images, metadata


def decode_metadata(metadata: np.ndarray) -> List[Dict]:
    """
    Decode metadata array into list of dictionaries
    
    Args:
        metadata: N×15 float32 array
        
    Returns:
        List of metadata dictionaries (one per cluster)
    """
    plane_map = {0: 'U', 1: 'V', 2: 'X'}
    
    decoded = []
    for meta in metadata:
        is_es = bool(meta[14])
        # Determine interaction type: ES (1.0), CC (0.0 and known), or UNKNOWN
        if meta[14] > 0.5:
            interaction_type = 'ES'
        else:
            # Could be CC or UNKNOWN - we treat both as 0.0
            interaction_type = 'CC/UNKNOWN'
        
        decoded.append({
            'is_marley': bool(meta[0]),
            'is_main_track': bool(meta[1]),
            'true_pos': meta[2:5],  # [x, y, z] in cm
            'true_dir': meta[5:8],  # [dx, dy, dz] normalized
            'true_mom': meta[8:11],  # [px, py, pz] in GeV/c
            'true_nu_energy': float(meta[11]),  # MeV
            'true_particle_energy': float(meta[12]),  # MeV
            'plane': plane_map[int(meta[13])],
            'plane_id': int(meta[13]),
            'is_es_interaction': is_es,
            'interaction_type': interaction_type
        })
    
    return decoded


def load_dataset(data_dir: str, plane: Optional[str] = None, 
                 marley_only: bool = False,
                 main_track_only: bool = False) -> Tuple[np.ndarray, np.ndarray]:
    """
    Load all clusters from a directory of batched files
    
    Args:
        data_dir: Directory containing batch .npz files
        plane: Filter by plane ('U', 'V', 'X'), None for all
        marley_only: Only load Marley clusters
        main_track_only: Only load main track clusters
        
    Returns:
        (images, metadata)
        - images: N×128×16 float32 array
        - metadata: N×15 float32 array (raw metadata)
    """
    data_dir = Path(data_dir)
    
    # Find batch files
    if plane:
        pattern = f'clusters_plane{plane}_batch*.npz'
    else:
        pattern = 'clusters_plane*_batch*.npz'
    
    files = sorted(data_dir.glob(pattern))
    
    all_images = []
    all_metadata = []
    
    for f in files:
        data = np.load(f)
        batch_images = data['images']
        batch_metadata = data['metadata']
        
        # Apply filters
        for i in range(len(batch_images)):
            meta = batch_metadata[i]
            
            if marley_only and not bool(meta[0]):
                continue
            if main_track_only and not bool(meta[1]):
                continue
            
            all_images.append(batch_images[i])
            all_metadata.append(meta)
        
        data.close()
    
    if len(all_images) == 0:
        return np.array([]), np.array([])
    
    return np.array(all_images), np.array(all_metadata)


def get_dataset_info(data_dir: str) -> Dict:
    """
    Get summary information about a batched dataset
    
    Args:
        data_dir: Directory containing batch .npz files
        
    Returns:
        Dictionary with dataset statistics
    """
    data_dir = Path(data_dir)
    files = list(data_dir.glob('clusters_plane*_batch*.npz'))
    
    if not files:
        return {'total_files': 0, 'total_clusters': 0}
    
    # Count by plane
    plane_counts = {'U': 0, 'V': 0, 'X': 0}
    marley_count = 0
    main_track_count = 0
    es_count = 0
    cc_count = 0
    total_size = 0
    total_clusters = 0
    
    for f in files:
        total_size += f.stat().st_size
        data = np.load(f)
        metadata = data['metadata']
        
        n_clusters = len(metadata)
        total_clusters += n_clusters
        
        for meta in metadata:
            plane_id = int(meta[13])
            plane = {0: 'U', 1: 'V', 2: 'X'}[plane_id]
            plane_counts[plane] += 1
            
            if bool(meta[0]):
                marley_count += 1
            if bool(meta[1]):
                main_track_count += 1
            if bool(meta[14]):
                es_count += 1
            else:
                cc_count += 1
        
        data.close()
    
    return {
        'total_batch_files': len(files),
        'total_clusters': total_clusters,
        'plane_counts': plane_counts,
        'marley_count': marley_count,
        'main_track_count': main_track_count,
        'es_count': es_count,
        'cc_count': cc_count,
        'total_size_mb': total_size / 1024 / 1024,
        'avg_batch_size_mb': total_size / len(files) / 1024 / 1024,
        'avg_clusters_per_batch': total_clusters / len(files),
        'avg_bytes_per_cluster': total_size / total_clusters if total_clusters > 0 else 0
    }


if __name__ == '__main__':
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python load_batched_cluster_data.py <data_dir> [batch_file]")
        print("\nExamples:")
        print("  # Show dataset info")
        print("  python load_batched_cluster_data.py data/prod_es/images_es_valid_bg_tick3_ch2_min2_tot2_e2p0")
        print("\n  # Load and inspect specific batch file")
        print("  python load_batched_cluster_data.py data/prod_es/images_es_valid_bg_tick3_ch2_min2_tot2_e2p0 clusters_planeU_batch0000.npz")
        sys.exit(1)
    
    data_dir = sys.argv[1]
    
    if len(sys.argv) == 3:
        # Load specific batch file
        npz_file = Path(data_dir) / sys.argv[2]
        if not npz_file.exists():
            print(f"Error: File not found: {npz_file}")
            sys.exit(1)
        
        print(f"Loading: {npz_file}")
        images, metadata = load_batch(npz_file)
        
        print(f"\n📊 Batch Info:")
        print(f"  Number of clusters: {len(images)}")
        print(f"  Image shape: {images.shape}")
        print(f"  Metadata shape: {metadata.shape}")
        
        # Decode metadata
        decoded = decode_metadata(metadata)
        
        print(f"\n🔍 First 3 clusters:")
        for i, meta in enumerate(decoded[:3]):
            print(f"\n  Cluster {i}:")
            for key, value in meta.items():
                if isinstance(value, np.ndarray):
                    print(f"    {key}: {value}")
                else:
                    print(f"    {key}: {value}")
            
            # Show image stats
            img = images[i]
            print(f"    image_range: [{img.min():.1f}, {img.max():.1f}] ADC")
            print(f"    non_zero_pixels: {np.count_nonzero(img)}")
    
    else:
        # Show dataset info
        print(f"Dataset: {data_dir}")
        info = get_dataset_info(data_dir)
        
        print(f"\n📊 Dataset Statistics:")
        print(f"  Batch files: {info['total_batch_files']}")
        print(f"  Total clusters: {info['total_clusters']}")
        print(f"  Total size: {info['total_size_mb']:.2f} MB")
        print(f"  Avg batch size: {info['avg_batch_size_mb']:.2f} MB")
        print(f"  Avg clusters per batch: {info['avg_clusters_per_batch']:.0f}")
        print(f"  Avg bytes per cluster: {info['avg_bytes_per_cluster']:.0f} bytes ({info['avg_bytes_per_cluster']/1024:.2f} KB)")
        
        print(f"\n  Plane distribution:")
        for plane, count in info['plane_counts'].items():
            pct = 100 * count / info['total_clusters'] if info['total_clusters'] > 0 else 0
            print(f"    {plane}: {count} ({pct:.1f}%)")
        
        print(f"\n  Interaction types:")
        print(f"    ES: {info['es_count']} ({100*info['es_count']/info['total_clusters']:.1f}%)")
        print(f"    CC/UNKNOWN: {info['cc_count']} ({100*info['cc_count']/info['total_clusters']:.1f}%)")
        print(f"    (Note: 0.0 = CC or UNKNOWN interaction type)")
        
        print(f"\n  Marley clusters: {info['marley_count']} ({100*info['marley_count']/info['total_clusters']:.1f}%)")
        print(f"  Main track clusters: {info['main_track_count']} ({100*info['main_track_count']/info['total_clusters']:.1f}%)")
