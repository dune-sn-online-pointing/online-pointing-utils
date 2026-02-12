#!/bin/bash
#
# Test analyze_clusters application
# Tests both ES and CC samples
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$(dirname "${SCRIPT_DIR}")"
ROOT_DIR="$(dirname "${TEST_DIR}")"
BUILD_DIR="${ROOT_DIR}/build"

# Run from ROOT_DIR so parameter files are found
cd "${ROOT_DIR}"

echo "================================================"
echo "Testing: analyze_clusters"
echo "================================================"

# Ensure we have cluster files
if [ ! -f "test/test_clustering/output/test_es_tps_bktr0_clusters_tick3_ch2_min2_tot1_en0.root" ]; then
    echo "Running clustering test first..."
    bash test/test_clustering/test_clustering.sh
fi

# Test ES sample
echo "Testing ES analysis..."
${BUILD_DIR}/src/app/analyze_clusters \
    -j test/test_analysis/test_analyze_clusters_es.json

if [ -f "test/test_analysis/output/test_es_tps_bktr0_clusters_tick3_ch2_min2_tot1_en0.pdf" ]; then
    echo "✓ ES cluster analysis successful"
else
    echo "✗ ES cluster analysis failed - PDF not found"
    exit 1
fi

# Test CC sample
echo "Testing CC analysis..."
${BUILD_DIR}/src/app/analyze_clusters \
    -j test/test_analysis/test_analyze_clusters_cc.json

if [ -f "test/test_analysis/output/test_cc_tps_bktr0_clusters_tick3_ch2_min2_tot1_en0.pdf" ]; then
    echo "✓ CC cluster analysis successful"
else
    echo "✗ CC cluster analysis failed - PDF not found"
    exit 1
fi

echo "================================================"
echo "All analyze_clusters tests passed!"
echo "================================================"
