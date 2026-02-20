#!/bin/bash
#
# Minimal smoke test: run the normal pipeline driver (scripts/sequence.sh)
# with a single settings file (json/test_settings.json).
#

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"

GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

SETTINGS_JSON="json/test_settings.json"
OUTPUT_DIR="test/output"

if [[ "${1:-}" == "--clean" ]]; then
  echo "Cleaning previous test outputs..."
  rm -rf "${ROOT_DIR}/${OUTPUT_DIR}"
fi

mkdir -p "${ROOT_DIR}/${OUTPUT_DIR}"

echo -e "${BLUE}========================================${NC}"
echo "  Online Pointing Utils - Smoke Test"
echo -e "========================================${NC}"

cd "${ROOT_DIR}"

echo "Running sequence pipeline using ${SETTINGS_JSON}"
./scripts/sequence.sh \
  -s test_cc \
  -j "${SETTINGS_JSON}" \
  -bt -ab -mc -mm \
  --max-files 1 \
  --skip-files 0

echo -e "\n${GREEN}Smoke test completed.${NC}"
