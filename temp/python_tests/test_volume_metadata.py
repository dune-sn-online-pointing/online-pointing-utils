#!/usr/bin/env python3
"""Test script to check volume metadata"""
import numpy as np
import sys

if len(sys.argv) < 2:
    print("Usage: python test_volume_metadata.py <volume_file.npz>")
    sys.exit(1)

npz_file = sys.argv[1]
data = np.load(npz_file, allow_pickle=True)

print(f"\nLoading: {npz_file}")
print(f"\nKeys in file: {list(data.keys())}")

if 'metadata' in data:
    metadata = data['metadata']
    print(f"\nMetadata type: {type(metadata)}")
    print(f"Metadata shape: {metadata.shape}")
    
    # Extract the dict
    if hasattr(metadata, 'item'):
        metadata = metadata.item()
    elif len(metadata) > 0:
        metadata = metadata[0]
    
    print(f"\nMetadata contents:")
    for key, value in metadata.items():
        print(f"  {key}: {value}")

if 'image' in data:
    image = data['image']
    print(f"\nImage shape: {image.shape}")
    print(f"Image range: [{image.min():.2f}, {image.max():.2f}]")
    print(f"Image center pixel: {image[image.shape[0]//2, image.shape[1]//2]:.2f}")
