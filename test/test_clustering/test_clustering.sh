#!/bin/bash
#
# Test make_clusters application
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
echo "Testing: make_clusters"
echo "================================================"

# First ensure we have backtracked files
if [ ! -f "test/test_backtrack/output/test_es_tps.root" ]; then
    echo "Running backtracking test first..."
    bash test/test_backtrack/test_backtrack.sh
fi

# Test ES sample
echo "Testing ES clustering..."
${BUILD_DIR}/src/app/make_clusters \
    -j test/test_clustering/test_clustering_es.json \
    -i test/test_backtrack/output/test_es_tps.root

# The app creates a folder based on parameters, so we need to check there
CLUSTERS_FOLDER="clusters__tick3_ch2_min2_tot1_e0p0"
if [ -f "${CLUSTERS_FOLDER}/test_es_clusters.root" ]; then
    echo "✓ ES clustering successful"
    # Create a symlink in the expected test output location for downstream tests
    mkdir -p test/test_clustering/output
    ln -sf "../../${CLUSTERS_FOLDER}/test_es_clusters.root" test/test_clustering/output/test_es_clusters.root
else
    echo "✗ ES clustering failed - output file not found"
    exit 1
fi

# Test CC sample
echo "Testing CC clustering..."
${BUILD_DIR}/src/app/make_clusters \
    -j test/test_clustering/test_clustering_cc.json \
    -i test/test_backtrack/output/test_cc_tps.root

if [ -f "${CLUSTERS_FOLDER}/test_cc_clusters.root" ]; then
    echo "✓ CC clustering successful"
    # Create a symlink for consistency
    ln -sf "../../${CLUSTERS_FOLDER}/test_cc_clusters.root" test/test_clustering/output/test_cc_clusters.root
else
    echo "✗ CC clustering failed - output file not found"
    exit 1
fi

echo "================================================"
echo "All clustering tests passed!"
echo "================================================"
