#!/bin/bash
#
# Integration test - runs a mini end-to-end pipeline
# Tests: backtrack → make_clusters → match_clusters → analyze
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$(dirname "${SCRIPT_DIR}")")"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}================================================${NC}"
echo -e "${YELLOW}Integration Test: Full Pipeline${NC}"
echo -e "${YELLOW}================================================${NC}"

# Setup paths
TEST_JSON="${SCRIPT_DIR}/test_integration.json"
OUTPUT_DIR="${SCRIPT_DIR}/output"

# Clean previous output
echo "Cleaning previous test output..."
rm -rf "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}"
mkdir -p "${OUTPUT_DIR}/reports"

# Check that test data exists
if [ ! -f "${ROOT_DIR}/test/data/test_es_tpstream.root" ]; then
    echo -e "${RED}✗ Test data not found: test/data/test_es_tpstream.root${NC}"
    exit 1
fi

cd "${ROOT_DIR}"

# Step 1: Backtrack
echo -e "\n${YELLOW}→ Step 1: Backtracking TPs...${NC}"
./build/src/app/backtrack_tpstream -j "${TEST_JSON}" --override || {
    echo -e "${RED}✗ Backtracking failed${NC}"
    exit 1
}
echo -e "${GREEN}✓ Backtracking completed${NC}"

# Verify backtrack output
if [ ! -f "${OUTPUT_DIR}/test_es_tps_bktr0.root" ]; then
    echo -e "${RED}✗ Backtrack output not found${NC}"
    exit 1
fi

# Step 2: Make clusters
echo -e "\n${YELLOW}→ Step 2: Creating clusters...${NC}"
./build/src/app/make_clusters -j "${TEST_JSON}" --override || {
    echo -e "${RED}✗ Clustering failed${NC}"
    exit 1
}
echo -e "${GREEN}✓ Clustering completed${NC}"

# Verify clustering output
CLUSTER_DIR=$(find "${OUTPUT_DIR}" -type d -name "*integration_clusters_tick*" | head -1)
if [ -z "${CLUSTER_DIR}" ]; then
    echo -e "${RED}✗ Cluster directory not found${NC}"
    exit 1
fi

CLUSTER_FILE="${CLUSTER_DIR}/test_es_clusters.root"
if [ ! -f "${CLUSTER_FILE}" ]; then
    echo -e "${RED}✗ Cluster file not found: ${CLUSTER_FILE}${NC}"
    exit 1
fi

# Step 3: Match clusters
echo -e "\n${YELLOW}→ Step 3: Matching clusters across planes...${NC}"
./build/src/app/match_clusters -j "${TEST_JSON}" --override || {
    echo -e "${RED}✗ Matching failed${NC}"
    exit 1
}
echo -e "${GREEN}✓ Matching completed${NC}"

# Verify matching output
MATCHED_DIR=$(find "${OUTPUT_DIR}" -type d -name "*matched_clusters_tick*" | head -1)
if [ -z "${MATCHED_DIR}" ]; then
    echo -e "${RED}✗ Matched cluster directory not found${NC}"
    exit 1
fi

MATCHED_FILE="${MATCHED_DIR}/test_es_matched.root"
if [ ! -f "${MATCHED_FILE}" ]; then
    echo -e "${RED}✗ Matched cluster file not found: ${MATCHED_FILE}${NC}"
    exit 1
fi

# Step 4: Analyze clusters
echo -e "\n${YELLOW}→ Step 4: Analyzing clusters...${NC}"
./build/src/app/analyze_clusters -j "${TEST_JSON}" || {
    echo -e "${RED}✗ Analysis failed${NC}"
    exit 1
}
echo -e "${GREEN}✓ Analysis completed${NC}"

# Verify analysis output  
REPORT_PDF=$(find "${OUTPUT_DIR}/reports" -name "*report.pdf" | head -1)
if [ -z "${REPORT_PDF}" ]; then
    echo -e "${YELLOW}⚠ Analysis report not found (this is OK if analysis produces no output)${NC}"
fi

# Summary
echo -e "\n${GREEN}================================================${NC}"
echo -e "${GREEN}Integration Test: PASSED${NC}"
echo -e "${GREEN}================================================${NC}"
echo -e "Output directory: ${OUTPUT_DIR}"
echo -e "Test completed successfully!"
echo ""

exit 0
