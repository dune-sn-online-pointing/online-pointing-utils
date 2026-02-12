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

# Run from ROOT_DIR so parameter files are found
cd "${ROOT_DIR}"

# Run from ROOT_DIR so parameter files are found
cd "${ROOT_DIR}"

echo "================================================"
echo "Testing: backtrack_tpstream"
echo "================================================"

# Test ES sample
echo "Testing ES sample..."
${BUILD_DIR}/src/app/backtrack \
    -j test/test_backtrack/test_backtrack_es.json \
    --output-folder test/test_backtrack/output

if [ -f "test/test_backtrack/output/test_es_tps_bktr0.root" ]; then
    echo "✓ ES backtracking successful"
    # Create symlink without _bktr0 suffix for convenience
    ln -sf test_es_tps_bktr0.root test/test_backtrack/output/test_es_tps.root
else
    echo "✗ ES backtracking failed - output file not found"
    exit 1
fi

# Test CC sample
echo "Testing CC sample..."
${BUILD_DIR}/src/app/backtrack \
    -j test/test_backtrack/test_backtrack_cc.json \
    --output-folder test/test_backtrack/output

if [ -f "test/test_backtrack/output/test_cc_tps_bktr0.root" ]; then
    echo "✓ CC backtracking successful"
    # Create symlink without _bktr0 suffix for convenience
    ln -sf test_cc_tps_bktr0.root test/test_backtrack/output/test_cc_tps.root
else
    echo "✗ CC backtracking failed - output file not found"
    exit 1
fi

echo "================================================"
echo "All backtracking tests passed!"
echo "================================================"
