#!/bin/bash
#
# Test backtrack_tpstream application
# Tests both ES and CC samples
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$(dirname "${SCRIPT_DIR}")"
ROOT_DIR="$(dirname "${TEST_DIR}")"
BUILD_DIR="${ROOT_DIR}/build"

echo "================================================"
echo "Testing: backtrack_tpstream"
echo "================================================"

# Test ES sample
echo "Testing ES sample..."
${BUILD_DIR}/src/app/backtrack_tpstream \
    -j ${SCRIPT_DIR}/test_backtrack_es.json

if [ -f "${SCRIPT_DIR}/output/test_es_tps.root" ]; then
    echo "✓ ES backtracking successful"
else
    echo "✗ ES backtracking failed - output file not found"
    exit 1
fi

# Test CC sample
echo "Testing CC sample..."
${BUILD_DIR}/src/app/backtrack_tpstream \
    -j ${SCRIPT_DIR}/test_backtrack_cc.json

if [ -f "${SCRIPT_DIR}/output/test_cc_tps.root" ]; then
    echo "✓ CC backtracking successful"
else
    echo "✗ CC backtracking failed - output file not found"
    exit 1
fi

echo "================================================"
echo "All backtracking tests passed!"
echo "================================================"
