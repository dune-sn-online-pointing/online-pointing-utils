#!/bin/bash
#
# Run all application tests
# This ensures nothing broke after code changes
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================"
echo "  Online Pointing Utils - Test Suite"
echo -e "========================================${NC}"
echo ""

# Parse arguments
CLEAN=false
if [[ "$1" == "--clean" ]]; then
    CLEAN=true
    echo "Cleaning previous test outputs..."
    rm -rf ${SCRIPT_DIR}/*/output
fi

# Track results
TESTS_PASSED=0
TESTS_FAILED=0
FAILED_TESTS=()

run_test_suite() {
    local suite_name="$1"
    local test_script="$2"
    
    echo -e "\n${YELLOW}→ Running ${suite_name}...${NC}"
    
    if ${test_script}; then
        ((TESTS_PASSED++))
        return 0
    else
        echo -e "${RED}✗ ${suite_name} FAILED${NC}"
        ((TESTS_FAILED++))
        FAILED_TESTS+=("${suite_name}")
        return 1
    fi
}

# Create output directories
mkdir -p ${SCRIPT_DIR}/test_backtrack/output
mkdir -p ${SCRIPT_DIR}/test_clustering/output
mkdir -p ${SCRIPT_DIR}/test_matching/output
mkdir -p ${SCRIPT_DIR}/test_analysis/output

# Run test suites
echo -e "${BLUE}Running test suites...${NC}"

run_test_suite "Backtracking" "${SCRIPT_DIR}/test_backtrack/test_backtrack.sh" || true
run_test_suite "Clustering" "${SCRIPT_DIR}/test_clustering/test_clustering.sh" || true
run_test_suite "Matching" "${SCRIPT_DIR}/test_matching/test_matching.sh" || true
run_test_suite "Analysis (clusters)" "${SCRIPT_DIR}/test_analysis/test_analyze_clusters.sh" || true
run_test_suite "Analysis (TPs)" "${SCRIPT_DIR}/test_analysis/test_analyze_tps.sh" || true
run_test_suite "Analysis (matching)" "${SCRIPT_DIR}/test_analysis/test_analyze_matching.sh" || true

# Print summary
echo ""
echo -e "${BLUE}========================================"
echo "  Test Summary"
echo -e "========================================${NC}"
echo -e "Passed: ${GREEN}${TESTS_PASSED}${NC}"
echo -e "Failed: ${RED}${TESTS_FAILED}${NC}"

if [ ${TESTS_FAILED} -gt 0 ]; then
    echo ""
    echo -e "${RED}Failed tests:${NC}"
    for test in "${FAILED_TESTS[@]}"; do
        echo -e "  ${RED}✗${NC} ${test}"
    done
    echo ""
    exit 1
else
    echo ""
    echo -e "${GREEN}All tests passed!${NC}"
    echo ""
    exit 0
fi
