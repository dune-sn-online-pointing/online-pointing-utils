#!/bin/bash
#
# Test analyze_tps application
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
echo "Testing: analyze_tps"
echo "================================================"

# Ensure we have backtracked TPs files
if [ ! -f "test/test_backtrack/output/test_es_tps_bktr0.root" ]; then
    echo "Running backtracking test first..."
    bash test/test_backtrack/test_backtrack.sh
fi

# Test ES sample
echo "Testing ES TPs analysis..."
${BUILD_DIR}/src/app/analyze_tps \
    -j test/test_analysis/test_analyze_tps_es.json

if [ -f "test/test_analysis/output/test_es_tps_bktr0_tot0.pdf" ]; then
    echo "✓ ES TPs analysis successful"
else
    echo "✗ ES TPs analysis failed - PDF not found"
    exit 1
fi

# Test CC sample
echo "Testing CC TPs analysis..."
${BUILD_DIR}/src/app/analyze_tps \
    -j test/test_analysis/test_analyze_tps_cc.json

if [ -f "test/test_analysis/output/test_cc_tps_bktr0_tot0.pdf" ]; then
    echo "✓ CC TPs analysis successful"
else
    echo "✗ CC TPs analysis failed - PDF not found"
    exit 1
fi

echo "================================================"
echo "All analyze_tps tests passed!"
echo "================================================"
