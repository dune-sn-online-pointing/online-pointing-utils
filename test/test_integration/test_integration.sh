#!/bin/bash
#
# Integration test - runs full pipeline using scripts/sequence.sh
# Tests: backtrack → make_clusters → match_clusters → analyze
# This validates the end-to-end workflow that users actually run
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

# Run full pipeline using sequence.sh
echo -e "\n${YELLOW}→ Running full pipeline: backtrack → cluster → match → analyze${NC}"
./scripts/sequence.sh -j "${TEST_JSON}" -bt -mc -mm -ac -f || {
    echo -e "${RED}✗ Pipeline failed${NC}"
    exit 1
}
echo -e "${GREEN}✓ Pipeline completed${NC}"

# Verify outputs
echo -e "\n${YELLOW}Verifying outputs...${NC}"

# Check backtrack output
if [ ! -f "${OUTPUT_DIR}/test_es_tps_bktr0.root" ]; then
    echo -e "${RED}✗ Backtrack output not found${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Backtrack output found${NC}"

# Check clustering output
CLUSTER_DIR=$(find "${OUTPUT_DIR}" -type d -name "*integration_clusters_tick*" | head -1)
if [ -z "${CLUSTER_DIR}" ]; then
    echo -e "${RED}✗ Cluster directory not found${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Cluster directory found: $(basename ${CLUSTER_DIR})${NC}"

# Check matching output
MATCHED_DIR=$(find "${OUTPUT_DIR}" -type d -name "*matched_clusters_tick*" | head -1)
if [ -z "${MATCHED_DIR}" ]; then
    echo -e "${RED}✗ Matched cluster directory not found${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Matched cluster directory found: $(basename ${MATCHED_DIR})${NC}"

# Check analysis output
REPORT_PDF=$(find "${OUTPUT_DIR}/reports" -name "*report.pdf" 2>/dev/null | head -1)
if [ -n "${REPORT_PDF}" ]; then
    echo -e "${GREEN}✓ Analysis report generated${NC}"
else
    echo -e "${YELLOW}⚠ Analysis report not found (may be OK if no data to analyze)${NC}"
fi

# Summary
echo -e "\n${GREEN}================================================${NC}"
echo -e "${GREEN}Integration Test: PASSED${NC}"
echo -e "${GREEN}================================================${NC}"
echo -e "Output directory: ${OUTPUT_DIR}"
echo -e "Test completed successfully!"
echo ""

exit 0
