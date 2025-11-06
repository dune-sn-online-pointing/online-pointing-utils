#!/bin/bash
#
# Test match_clusters application
# Tests both ES and CC samples
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$(dirname "${SCRIPT_DIR}")"
ROOT_DIR="$(dirname "${TEST_DIR}")"
BUILD_DIR="${ROOT_DIR}/build"

echo "================================================"
echo "Testing: match_clusters"
echo "================================================"

# First ensure we have cluster files
if [ ! -f "../test_clustering/output/test_es_clusters.root" ]; then
    echo "Running clustering test first..."
    cd ../test_clustering && ./test_clustering.sh && cd - > /dev/null
fi

# Test ES sample
echo "Testing ES matching..."
${BUILD_DIR}/src/app/match_clusters \
    -j ${SCRIPT_DIR}/test_matching_es.json

if [ -f "${SCRIPT_DIR}/output/test_es_matched.root" ]; then
    echo "✓ ES matching successful"
else
    echo "✗ ES matching failed - output file not found"
    exit 1
fi

# Test CC sample
echo "Testing CC matching..."
${BUILD_DIR}/src/app/match_clusters \
    -j ${SCRIPT_DIR}/test_matching_cc.json

if [ -f "${SCRIPT_DIR}/output/test_cc_matched.root" ]; then
    echo "✓ CC matching successful"
else
    echo "✗ CC matching failed - output file not found"
    exit 1
fi

echo "================================================"
echo "All matching tests passed!"
echo "================================================"
