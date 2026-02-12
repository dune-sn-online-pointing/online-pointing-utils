#!/usr/bin/env python3
"""
Integration test: Load a real ROOT file and process clusters without GUI
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent / 'python'))

def test_load_root_file(clusters_file):
    """Test loading a real ROOT file"""
    print(f"Testing with: {Path(clusters_file).name}")
    print("=" * 60)
    
    try:
        from cluster_display import ClusterViewer, Parameters
        import uproot
        
        # Check file exists
        if not Path(clusters_file).exists():
            print(f"✗ File not found: {clusters_file}")
            return False
        
        print(f"✓ File exists")
        
        # Try to open with uproot
        print("\nOpening ROOT file...")
        with uproot.open(clusters_file) as f:
            print(f"✓ ROOT file opened successfully")
            
            # Check for clusters directory
            if 'clusters' not in f:
                print("✗ No 'clusters' directory found")
                return False
            print("✓ 'clusters' directory found")
            
            # Check for trees
            clusters_dir = f['clusters']
            trees = [key for key in clusters_dir.keys() if 'clusters_tree' in key]
            print(f"✓ Found {len(trees)} cluster trees: {trees}")
        
        # Test loading with ClusterViewer (without showing GUI)
        print("\nInitializing ClusterViewer...")
        viewer = ClusterViewer(
            clusters_file=clusters_file,
            mode='clusters',
            draw_mode='pentagon',
            only_marley=False,
            params_dir="parameters"
        )
        
        print(f"✓ ClusterViewer initialized")
        print(f"  Total items loaded: {len(viewer.items)}")
        
        if len(viewer.items) > 0:
            item = viewer.items[0]
            print(f"\nFirst cluster info:")
            print(f"  Plane: {item.plane}")
            print(f"  Channel: {item.channel}")
            print(f"  Time start: {item.time_start:.2f}")
            print(f"  Time peak: {item.time_peak:.2f}")
            print(f"  Time end: {item.time_end:.2f}")
            print(f"  ADC peak: {item.adc_peak:.2f}")
            print(f"  ADC integral: {item.adc_integral:.2f}")
            print(f"  ✓ First cluster loaded successfully")
        else:
            print("⚠ No clusters found in file")
        
        return True
        
    except Exception as e:
        print(f"✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    """Run integration test"""
    print("=" * 60)
    print("cluster_display.py Integration Test")
    print("=" * 60)
    print()
    
    # Use first available cluster file
    clusters_file = "data/prodmarley_nue_flat_es_dune10kt_1x2x2_20250926T162658Z_gen_000283_supernova_g4_detsim_ana_2025-10-03T_050358Zbktr10_clusters_tick5_ch2_min2_tot1_en0.root"
    
    success = test_load_root_file(clusters_file)
    
    print("\n" + "=" * 60)
    if success:
        print("✓ Integration test PASSED")
        print("\nYou can now run the full GUI with:")
        print(f"  python3 python/cluster_display.py --clusters-file <file.root> --draw-mode pentagon")
        return 0
    else:
        print("✗ Integration test FAILED")
        return 1

if __name__ == '__main__':
    sys.exit(main())
