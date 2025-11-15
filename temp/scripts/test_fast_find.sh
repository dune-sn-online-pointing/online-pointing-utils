#!/bin/bash
# Test script to compare file finding performance

echo "============================================"
echo "Testing file finding performance"
echo "============================================"
echo ""

# Configuration
SIG_FOLDER="/eos/user/e/evilla/dune/sn-tps/production_es/fixed_tps/"
BG_FOLDER="/eos/user/e/evilla/dune/sn-tps/bkgs/"
JSON_FILE="/afs/cern.ch/work/e/evilla/private/dune/online-pointing-utils/json/es_fixed_myproc.json"

# Test 1: Run add_backgrounds with max_files=1 to see startup time
echo "Test 1: Running add_backgrounds with max_files=1 (only testing file finding speed)"
echo "Command: time ./build/src/app/add_backgrounds -j $JSON_FILE -m 1 -s 0"
echo ""
time ./build/src/app/add_backgrounds -j $JSON_FILE -m 1 -s 0 2>&1 | grep -E "(find_input_files|Found [0-9]+ signal|Found [0-9]+ background|30/10/2025)"

echo ""
echo "============================================"
echo "Test complete!"
echo "============================================"
echo ""
echo "Key metrics to look for:"
echo "  - Time between '[find_input_files] Called' and 'Found X signal files'"
echo "  - Time between finding signal files and finding background files"
echo "  - Total execution time"
echo ""
