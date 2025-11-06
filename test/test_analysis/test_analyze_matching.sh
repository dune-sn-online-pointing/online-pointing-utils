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

echo "================================================"
echo "Testing: analyze_matching"
echo "================================================"

# Ensure we have matched files
if [ ! -f "../test_matching/output/test_es_matched.root" ]; then
    echo "Running matching test first..."
    cd ../test_matching && ./test_matching.sh && cd - > /dev/null
fi

# Test ES sample
echo "Testing ES matching analysis..."
${BUILD_DIR}/src/app/analyze_matching \
    -j ${SCRIPT_DIR}/test_analyze_matching_es.json

echo "✓ ES matching analysis completed"

# Test CC sample
echo "Testing CC matching analysis..."
${BUILD_DIR}/src/app/analyze_matching \
    -j ${SCRIPT_DIR}/test_analyze_matching_cc.json

echo "✓ CC matching analysis completed"

echo "================================================"
echo "All analyze_matching tests passed!"
echo "================================================"
