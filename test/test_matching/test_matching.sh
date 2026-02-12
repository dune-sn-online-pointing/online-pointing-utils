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

# Run from ROOT_DIR so parameter files are found
cd "${ROOT_DIR}"

echo "================================================"
echo "Testing: match_clusters"
echo "================================================"

# Test ES sample
echo "Testing ES matching..."
${BUILD_DIR}/src/app/match_clusters \
    -j test/test_matching/test_matching_es.json

if [ -f "test/test_matching/output/test_es_clusters_u.root" ] && \
   [ -f "test/test_matching/output/test_es_clusters_v.root" ] && \
   [ -f "test/test_matching/output/test_es_clusters_x.root" ]; then
    echo "✓ ES matching successful"
else
    echo "✗ ES matching failed - output files not found"
    exit 1
fi

# Test CC sample
echo "Testing CC matching..."
${BUILD_DIR}/src/app/match_clusters \
    -j test/test_matching/test_matching_cc.json

if [ -f "test/test_matching/output/test_cc_clusters_u.root" ] && \
   [ -f "test/test_matching/output/test_cc_clusters_v.root" ] && \
   [ -f "test/test_matching/output/test_cc_clusters_x.root" ]; then
    echo "✓ CC matching successful"
else
    echo "✗ CC matching failed - output files not found"
    exit 1
fi

echo "================================================"
echo "All matching tests passed!"
echo "================================================"
