#!/bin/bash
#
# Test analyze_clusters application
# Tests both ES and CC samples
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$(dirname "${SCRIPT_DIR}")"
ROOT_DIR="$(dirname "${TEST_DIR}")"
BUILD_DIR="${ROOT_DIR}/build"

echo "================================================"
echo "Testing: analyze_clusters"
echo "================================================"

# Ensure we have cluster files
if [ ! -f "../test_clustering/output/test_es_clusters.root" ]; then
    echo "Running clustering test first..."
    cd ../test_clustering && ./test_clustering.sh && cd - > /dev/null
fi

# Test ES sample
echo "Testing ES analysis..."
${BUILD_DIR}/src/app/analyze_clusters \
    -j ${SCRIPT_DIR}/test_analyze_clusters_es.json

if [ -f "${SCRIPT_DIR}/output/test_es_clusters_report.pdf" ]; then
    echo "✓ ES cluster analysis successful"
else
    echo "✗ ES cluster analysis failed - PDF not found"
    exit 1
fi

# Test CC sample
echo "Testing CC analysis..."
${BUILD_DIR}/src/app/analyze_clusters \
    -j ${SCRIPT_DIR}/test_analyze_clusters_cc.json

if [ -f "${SCRIPT_DIR}/output/test_cc_clusters_report.pdf" ]; then
    echo "✓ CC cluster analysis successful"
else
    echo "✗ CC cluster analysis failed - PDF not found"
    exit 1
fi

echo "================================================"
echo "All analyze_clusters tests passed!"
echo "================================================"
