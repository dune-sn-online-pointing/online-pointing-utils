#!/bin/bash
#
# Comprehensive test script for online-pointing-utils
# Tests all main applications with sample data to verify nothing broke
#
# Usage: ./run_tests.sh [--clean]
#   --clean: Remove test output before running tests

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="${SCRIPT_DIR}"
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
BUILD_DIR="${ROOT_DIR}/build"
TEST_OUTPUT="${TEST_DIR}/test_output"
TEST_CONFIGS="${TEST_DIR}/configs"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "  Online Pointing Utils - Test Suite"
echo "========================================"
echo ""

# Clean if requested
if [[ "$1" == "--clean" ]]; then
    echo "Cleaning previous test outputs..."
    rm -rf "${TEST_OUTPUT}"
    rm -rf "${TEST_CONFIGS}"
fi

# Create output and config directories
mkdir -p "${TEST_OUTPUT}"
mkdir -p "${TEST_CONFIGS}"

# Track test results
TESTS_PASSED=0
TESTS_FAILED=0
FAILED_TESTS=()

# Helper function to run a test
run_test() {
    local test_name="$1"
    local test_command="$2"
    
    echo -n "Testing ${test_name}... "
    
    if eval "${test_command}" > "${TEST_OUTPUT}/${test_name}.log" 2>&1; then
        echo -e "${GREEN}✓ PASS${NC}"
        ((TESTS_PASSED++))
        return 0
    else
        echo -e "${RED}✗ FAIL${NC}"
        echo "  See ${TEST_OUTPUT}/${test_name}.log for details"
        ((TESTS_FAILED++))
        FAILED_TESTS+=("${test_name}")
        return 1
    fi
}

# Create test JSON configurations
echo "Setting up test configurations..."

# Test 1: Backtracking
cat > "${TEST_CONFIGS}/test_backtrack.json" <<EOF
{
  "tpstream_input_file": "${TEST_DIR}/test_es_file_tpstream.root",
  "outputFolder": "${TEST_OUTPUT}/",
  "backtracker_error_margin": 50
}
EOF

# Test 2: Add Backgrounds (uses renamed backtracked file)
cat > "${TEST_CONFIGS}/test_add_backgrounds.json" <<EOF
{
  "signal_type": "es",
  "sig_folder": "${TEST_OUTPUT}/",
  "sig_input_file": "${TEST_OUTPUT}/test_es_file_tps.root",
  "bg_folder": "${TEST_DIR}/",
  "tps_bg_folder": "${TEST_OUTPUT}/",
  "around_vertex_only": false,
  "vertex_radius": 100,
  "max_files": 1
}
EOF

# Test 3: Clustering (uses renamed background-merged file)
cat > "${TEST_CONFIGS}/test_clustering.json" <<EOF
{
  "tps_folder": "${TEST_OUTPUT}/",
  "tps_input_file": "${TEST_OUTPUT}/test_es_file_tps_bg.root",
  "outputFolder": "${TEST_OUTPUT}/",
  "tick_limit": 3,
  "channel_limit": 2,
  "min_tps_to_cluster": 2,
  "tot_cut": 3,
  "energy_cut": 0,
  "max_files": 1
}
EOF

# Test 4: Analysis (specify cluster file directly)
cat > "${TEST_CONFIGS}/test_analysis.json" <<EOF
{
  "clusters_folder": "${ROOT_DIR}/clusters_clusters_tick3_ch2_min2_tot3_e0p0/",
  "clusters_input_file": "${ROOT_DIR}/clusters_clusters_tick3_ch2_min2_tot3_e0p0/clusters_0.root",
  "outputFolder": "${TEST_OUTPUT}/",
  "max_files": 1,
  "generate_combined_pdf": true
}
EOF

echo ""
echo "Running application tests..."
echo "----------------------------"

# Test backtracking
run_test "backtrack_tpstream" \
    "cd ${ROOT_DIR} && ${BUILD_DIR}/src/app/backtrack_tpstream -j ${TEST_CONFIGS}/test_backtrack.json"

# Rename backtracked file to match expected pattern for add_backgrounds
mv "${TEST_OUTPUT}/test_es_file_tps_bktr50.root" "${TEST_OUTPUT}/test_es_file_tps.root" 2>/dev/null || true

# Test add_backgrounds
run_test "add_backgrounds" \
    "cd ${ROOT_DIR} && ${BUILD_DIR}/src/app/add_backgrounds -j ${TEST_CONFIGS}/test_add_backgrounds.json"

# Rename background-merged file to match expected pattern for make_clusters
mv "${TEST_OUTPUT}/test_es_file_bg_tps.root" "${TEST_OUTPUT}/test_es_file_tps_bg.root" 2>/dev/null || true

# Test clustering
run_test "make_clusters" \
    "cd ${ROOT_DIR} && ${BUILD_DIR}/src/app/make_clusters -j ${TEST_CONFIGS}/test_clustering.json"

# Test cluster analysis
run_test "analyze_clusters" \
    "cd ${ROOT_DIR} && ${BUILD_DIR}/src/app/analyze_clusters -j ${TEST_CONFIGS}/test_analysis.json"

echo ""
echo "Running Python analysis tests..."
echo "--------------------------------"

# Test Python analysis scripts
if [[ -f "${TEST_OUTPUT}/clusters_tick3_ch2_min2_tot3_e0p0/clusters_0.root" ]]; then
    run_test "python_cluster_analysis" \
        "cd ${ROOT_DIR} && python3 python/ana/analyze_clusters.py ${TEST_OUTPUT}/clusters_tick3_ch2_min2_tot3_e0p0/clusters_0.root ${TEST_OUTPUT}/test_python_analysis.txt"
else
    echo -e "${YELLOW}⊘ SKIP${NC} python_cluster_analysis (no cluster file)"
fi

# Verify output files exist
echo ""
echo "Verifying outputs..."
echo "--------------------"

verify_file() {
    local file_pattern="$1"
    local description="$2"
    
    if ls ${file_pattern} 1> /dev/null 2>&1; then
        echo -e "${GREEN}✓${NC} Found: ${description}"
        return 0
    else
        echo -e "${RED}✗${NC} Missing: ${description}"
        return 1
    fi
}

verify_file "${TEST_OUTPUT}/*_tps_bktr50.root" "Backtracked TPs"
verify_file "${TEST_OUTPUT}/*_bg.root" "Background-merged TPs"
verify_file "${TEST_OUTPUT}/clusters_*/*.root" "Cluster files"
verify_file "${TEST_OUTPUT}/*_analysis.pdf" "Analysis PDF" || true

# Summary
echo ""
echo "========================================"
echo "  Test Summary"
echo "========================================"
echo -e "Tests passed: ${GREEN}${TESTS_PASSED}${NC}"
echo -e "Tests failed: ${RED}${TESTS_FAILED}${NC}"

if [[ ${TESTS_FAILED} -gt 0 ]]; then
    echo ""
    echo "Failed tests:"
    for test in "${FAILED_TESTS[@]}"; do
        echo -e "  ${RED}✗${NC} ${test}"
    done
    echo ""
    echo "Check log files in ${TEST_OUTPUT}/ for details"
    exit 1
else
    echo ""
    echo -e "${GREEN}All tests passed!${NC}"
    echo ""
    echo "Test outputs saved to: ${TEST_OUTPUT}/"
    exit 0
fi
