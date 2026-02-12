#!/usr/bin/env python3
"""
Test script to verify get_clusters_folder() matches C++ implementation
"""

import sys
import json
from pathlib import Path

# Add scripts directory to path
sys.path.insert(0, str(Path(__file__).parent))

from generate_cluster_arrays import get_clusters_folder

def test_clusters_folder():
    """Test various configurations"""
    
    # Test case 1: es_valid.json example
    config = {
        "clusters_folder": "/home/virgolaema/dune/online-pointing-utils/data/prod_es/",
        "clusters_folder_prefix": "es_valid_bg",
        "tick_limit": 3,
        "channel_limit": 2,
        "min_tps_to_cluster": 2,
        "tot_cut": 1,
        "energy_cut": 0
    }
    
    result = get_clusters_folder(config)
    expected = "/home/virgolaema/dune/online-pointing-utils/data/prod_es/clusters_es_valid_bg_tick3_ch2_min2_tot1_e0"
    
    print("Test 1: es_valid.json configuration")
    print(f"  Result:   {result}")
    print(f"  Expected: {expected}")
    print(f"  Match: {result == expected}")
    print()
    
    # Test case 2: Floating point energy
    config2 = {
        "clusters_folder": "/data/",
        "clusters_folder_prefix": "test",
        "tick_limit": 5,
        "channel_limit": 2,
        "min_tps_to_cluster": 3,
        "tot_cut": 3,
        "energy_cut": 2.5
    }
    
    result2 = get_clusters_folder(config2)
    expected2 = "/data/clusters_test_tick5_ch2_min3_tot3_e2p5"
    
    print("Test 2: Floating point energy (2.5)")
    print(f"  Result:   {result2}")
    print(f"  Expected: {expected2}")
    print(f"  Match: {result2 == expected2}")
    print()
    
    # Test case 3: Default values
    config3 = {}
    
    result3 = get_clusters_folder(config3)
    expected3 = "./clusters_clusters_tick0_ch0_min0_tot0_e0"
    
    print("Test 3: All defaults")
    print(f"  Result:   {result3}")
    print(f"  Expected: {expected3}")
    print(f"  Match: {result3 == expected3}")
    print()

if __name__ == '__main__':
    test_clusters_folder()
