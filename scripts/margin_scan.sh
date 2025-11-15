#!/bin/bash

# Backtracking Margin Scan Script
# Tests margins 0, 5, 10, 15 on clean and background samples
# to optimize contamination vs efficiency tradeoff

set -e  # Exit on any error

echo "======================================="
echo "DUNE Backtracking Margin Scan"
echo "Testing margins: 0, 5, 10, 15"
echo "Samples: cleanES909080, cleanES60, bkgES909080, bkgES60"
echo "======================================="

# Define samples and margins
samples=("cleanES909080" "cleanES60" "bkgES909080" "bkgES60")
margins=(0 5 10 15)

# Parse verbose flag from command line
VERBOSE=false
for arg in "$@"; do
  case "$arg" in
    -v|--verbose) VERBOSE=true;;
  esac
done

# Build directory
BUILD_DIR="/home/virgolaema/dune/online-pointing-utils/build"
JSON_DIR="/home/virgolaema/dune/online-pointing-utils/json"

# Results summary file
RESULTS_FILE="data/margin_scan_results_$(date +%Y%m%d_%H%M%S).txt"
echo "Backtracking Margin Scan Results - $(date)" > "$RESULTS_FILE"
echo "=========================================" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"

# Loop through all combinations
for sample in "${samples[@]}"; do
    echo ""
    echo "Processing sample: $sample"
    echo "=================================="
    
    echo "Sample: $sample" >> "$RESULTS_FILE"
    echo "----------------" >> "$RESULTS_FILE"
    
    for margin in "${margins[@]}"; do
        echo ""
        echo "  Running margin $margin..."
        
        # Create temporary JSON with the specific margin
        temp_json="${JSON_DIR}/backtrack/temp_${sample}_margin${margin}.json"
        base_json="${JSON_DIR}/backtrack/margin_scan_${sample}.json"
        
        # Modify the margin in the JSON
        sed "s/\"backtracker_error_margin\": [0-9]*/\"backtracker_error_margin\": $margin/" "$base_json" > "$temp_json"
        
        # Run backtracking
        echo "    Backtracking with margin $margin..."
        backtrack_cmd="$BUILD_DIR/src/app/backtrack -j \"$temp_json\""
        if [ "$VERBOSE" = true ]; then
            backtrack_cmd="$backtrack_cmd -v"
        fi
        eval $backtrack_cmd > "temp_backtrack_output.log" 2>&1
        
        # Extract output filename from log by looking for the expected pattern
        base_name=$(echo "$sample" | tr -d '\n')
        output_file="data/${base_name}_tps_bktr${margin}.root"
        
        if [ ! -f "$output_file" ]; then
            echo "    ERROR: Output file not found: $output_file"
            echo "    Margin $margin: ERROR - output file not created" >> "$RESULTS_FILE"
            continue
        fi
        
        # Create analyze_tps JSON
        analyze_json="${JSON_DIR}/analyze_tps/temp_${sample}_margin${margin}.json"
        cat > "$analyze_json" << EOF
{
  "filename": "$output_file",
  "output_folder": "data",
  "tot_cut": 0
}
EOF
        
        # Run analyze_tps 
        echo "    Analyzing results..."
        analyze_cmd="$BUILD_DIR/src/app/analyze_tps -j \"$analyze_json\""
        if [ "$VERBOSE" = true ]; then
            analyze_cmd="$analyze_cmd -v"
        fi
        eval $analyze_cmd > "temp_analyze_output.log" 2>&1
        
        # Extract key metrics from analyze output
        marley_events=$(grep "MARLEY diagnostic:" "temp_analyze_output.log" | grep "events contain MARLEY" | sed 's/.*MARLEY diagnostic: \([0-9]*\)\/\([0-9]*\) events contain MARLEY.*/\1\/\2/')
        unknown_total=$(grep "UNKNOWN diagnostic:" "temp_analyze_output.log" | sed 's/.*total=\([0-9]*\).*/\1/')
        tp_counts=$(grep "Number of TPs after ToT cut" "temp_analyze_output.log" | sed 's/.*X: \([0-9]*\), U: \([0-9]*\), V: \([0-9]*\).*/X:\1 U:\2 V:\3/')
        offset_analysis=$(grep "Time offset analysis:" "temp_analyze_output.log" | sed 's/.*Time offset analysis: \(.*\)/\1/')
        
        # Log results
        echo "    Results: MARLEY=$marley_events, UNKNOWN=$unknown_total, TPs=($tp_counts)"
        
        echo "    Margin $margin: MARLEY=$marley_events UNKNOWN=$unknown_total TPs=($tp_counts) Offsets=($offset_analysis)" >> "$RESULTS_FILE"
        
        # Clean up temporary files
        rm -f "$temp_json" "$analyze_json"
    done
    
    echo "" >> "$RESULTS_FILE"
done

# Clean up logs
rm -f temp_backtrack_output.log temp_analyze_output.log

echo ""
echo "======================================="
echo "Scan completed! Results saved to: $RESULTS_FILE"
echo "======================================="

# Display summary
echo ""
echo "SUMMARY:"
cat "$RESULTS_FILE"
