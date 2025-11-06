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

echo "================================================"
echo "Testing: analyze_tps"
echo "================================================"

# Ensure we have backtracked TPs files
if [ ! -f "../test_backtrack/output/test_es_tps.root" ]; then
    echo "Running backtracking test first..."
    cd ../test_backtrack && ./test_backtrack.sh && cd - > /dev/null
fi

# Test ES sample
echo "Testing ES TPs analysis..."
${BUILD_DIR}/src/app/analyze_tps \
    -j ${SCRIPT_DIR}/test_analyze_tps_es.json

if [ -f "${SCRIPT_DIR}/output/test_es_tps_report.pdf" ]; then
    echo "✓ ES TPs analysis successful"
else
    echo "✗ ES TPs analysis failed - PDF not found"
    exit 1
fi

# Test CC sample
echo "Testing CC TPs analysis..."
${BUILD_DIR}/src/app/analyze_tps \
    -j ${SCRIPT_DIR}/test_analyze_tps_cc.json

if [ -f "${SCRIPT_DIR}/output/test_cc_tps_report.pdf" ]; then
    echo "✓ CC TPs analysis successful"
else
    echo "✗ CC TPs analysis failed - PDF not found"
    exit 1
fi

echo "================================================"
echo "All analyze_tps tests passed!"
echo "================================================"
