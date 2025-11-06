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

echo "================================================"
echo "Testing: make_clusters"
echo "================================================"

# First ensure we have backtracked files
if [ ! -f "../test_backtrack/output/test_es_tps.root" ]; then
    echo "Running backtracking test first..."
    cd ../test_backtrack && ./test_backtrack.sh && cd - > /dev/null
fi

# Test ES sample
echo "Testing ES clustering..."
${BUILD_DIR}/src/app/make_clusters \
    -j ${SCRIPT_DIR}/test_clustering_es.json

if [ -f "${SCRIPT_DIR}/output/test_es_clusters.root" ]; then
    echo "✓ ES clustering successful"
else
    echo "✗ ES clustering failed - output file not found"
    exit 1
fi

# Test CC sample
echo "Testing CC clustering..."
${BUILD_DIR}/src/app/make_clusters \
    -j ${SCRIPT_DIR}/test_clustering_cc.json

if [ -f "${SCRIPT_DIR}/output/test_cc_clusters.root" ]; then
    echo "✓ CC clustering successful"
else
    echo "✗ CC clustering failed - output file not found"
    exit 1
fi

echo "================================================"
echo "All clustering tests passed!"
echo "================================================"
