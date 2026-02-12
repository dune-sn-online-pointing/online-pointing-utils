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
${BUILD_DIR}/src/app/cluster \
    -j test/test_clustering/test_clustering_es.json \
    --output-folder test/test_clustering/output

ES_CLUSTER_FILE="test/test_clustering/output/test_es_tps_bktr0_clusters_tick3_ch2_min2_tot1_en0.root"
if [ -f "${ES_CLUSTER_FILE}" ]; then
    echo "✓ ES clustering successful"
else
    echo "✗ ES clustering failed - output file not found"
    exit 1
fi

# Test CC sample
echo "Testing CC clustering..."
${BUILD_DIR}/src/app/cluster \
    -j test/test_clustering/test_clustering_cc.json \
    --output-folder test/test_clustering/output

CC_CLUSTER_FILE="test/test_clustering/output/test_cc_tps_bktr0_clusters_tick3_ch2_min2_tot1_en0.root"
if [ -f "${CC_CLUSTER_FILE}" ]; then
    echo "✓ CC clustering successful"
else
    echo "✗ CC clustering failed - output file not found"
    exit 1
fi

echo "================================================"
echo "All clustering tests passed!"
echo "================================================"
