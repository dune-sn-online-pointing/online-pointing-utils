#!/bin/bash
#
# Test analyze_matching application
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
echo "Testing: analyze_matching"
echo "================================================"

# Ensure we have matched files
if [ ! -f "matched_clusters__tick3_ch2_min2_tot1_e0p0/test_es_matched.root" ]; then
    echo "Running matching test first..."
    bash test/test_matching/test_matching.sh
fi

# Test ES sample
echo "Testing ES matching analysis..."
${BUILD_DIR}/src/app/analyze_matching \
    -j test/test_analysis/test_analyze_matching_es.json

echo "✓ ES matching analysis completed"

# Test CC sample
echo "Testing CC matching analysis..."
${BUILD_DIR}/src/app/analyze_matching \
    -j test/test_analysis/test_analyze_matching_cc.json

echo "✓ CC matching analysis completed"

echo "================================================"
echo "All analyze_matching tests passed!"
echo "================================================"
