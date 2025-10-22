#!/bin/bash
# Script to run clustering and analysis for different energy cuts

# Energy cut values to test (in MeV)
ENERGY_CUTS=("0" "0.5" "1.0" "1.5" "2.0" "2.5" "3.0" "3.5" "4.0" "4.5" "5.0")

# CC simulations
echo "Running CC simulations..."
for ecut in "${ENERGY_CUTS[@]}"; do
    echo "Processing CC with energy_cut=$ecut MeV"
    
    # Create temporary JSON with this energy cut
    cat > /tmp/cc_temp.json << EOF
{
    "tpstream_folder": "/home/virgolaema/dune/online-pointing-utils/data/prod_cc/",
    "sig_folder": "/home/virgolaema/dune/online-pointing-utils/data/prod_cc/",
    "bg_folder": "/home/virgolaema/dune/online-pointing-utils/data/bkgs/",
    "tps_folder": "/home/virgolaema/dune/online-pointing-utils/data/prod_cc/bkgs/",
    "tps_bg_folder": "/home/virgolaema/dune/online-pointing-utils/data/prod_cc/bkgs/",
    "clusters_folder": "/home/virgolaema/dune/online-pointing-utils/data/prod_cc/",
    "clusters_folder_prefix": "cc_valid_bg",
    "reports_folder": "/home/virgolaema/dune/online-pointing-utils/data/prod_cc/reports/",
    "max_files": 20,
    "skip_files": 0,
    "backtracker_error_margin": 10,
    "tick_limit": 3,
    "channel_limit": 2,
    "min_tps_to_cluster": 2,
    "tot_cut": 2,
    "energy_cut": $ecut
}
EOF
    
    # Run clustering and analysis
    ./scripts/sequence.sh -j /tmp/cc_temp.json -mc -ac --no-compile
done

# ES simulations
echo "Running ES simulations..."
for ecut in "${ENERGY_CUTS[@]}"; do
    echo "Processing ES with energy_cut=$ecut MeV"
    
    # Create temporary JSON with this energy cut
    cat > /tmp/es_temp.json << EOF
{
    "tpstream_folder": "/home/virgolaema/dune/online-pointing-utils/data/prod_es/",
    "sig_folder": "/home/virgolaema/dune/online-pointing-utils/data/prod_es/",
    "bg_folder": "/home/virgolaema/dune/online-pointing-utils/data/bkgs/",
    "tps_folder": "/home/virgolaema/dune/online-pointing-utils/data/prod_es/bkgs/",
    "tps_bg_folder": "/home/virgolaema/dune/online-pointing-utils/data/prod_es/bkgs/",
    "clusters_folder": "/home/virgolaema/dune/online-pointing-utils/data/prod_es/",
    "clusters_folder_prefix": "es_valid_bg",
    "reports_folder": "/home/virgolaema/dune/online-pointing-utils/data/prod_es/reports/",
    "max_files": 20,
    "skip_files": 0,
    "backtracker_error_margin": 10,
    "tick_limit": 3,
    "channel_limit": 2,
    "min_tps_to_cluster": 2,
    "tot_cut": 2,
    "energy_cut": $ecut
}
EOF
    
    # Run clustering and analysis
    ./scripts/sequence.sh -j /tmp/es_temp.json -mc -ac --no-compile
done

echo "All simulations completed!"
