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
if [ ! -f "clusters__tick3_ch2_min2_tot1_e0p0/test_es_clusters.root" ]; then
    echo "Running clustering test first..."
    bash test/test_clustering/test_clustering.sh
fi

# Test ES sample
echo "Testing ES analysis..."
${BUILD_DIR}/src/app/analyze_clusters \
    -j test/test_analysis/test_analyze_clusters_es.json \
    -i clusters__tick3_ch2_min2_tot1_e0p0/test_es_clusters.root

if ls test/test_analysis/output/*.pdf >/dev/null 2>&1; then
    echo "✓ ES cluster analysis successful"
else
    echo "✗ ES cluster analysis failed - PDF not found"
    exit 1
fi

# Test CC sample
echo "Testing CC analysis..."
${BUILD_DIR}/src/app/analyze_clusters \
    -j test/test_analysis/test_analyze_clusters_cc.json \
    -i clusters__tick3_ch2_min2_tot1_e0p0/test_cc_clusters.root

if ls test/test_analysis/output/*.pdf >/dev/null 2>&1; then
    echo "✓ CC cluster analysis successful"
else
    echo "✗ CC cluster analysis failed - PDF not found"
    exit 1
fi

echo "================================================"
echo "All analyze_clusters tests passed!"
echo "================================================"
