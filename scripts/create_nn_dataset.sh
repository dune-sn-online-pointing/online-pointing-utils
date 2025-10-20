#!/bin/bash

set -e
export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPTS_DIR/init.sh

print_help(){
  echo "Usage: $0 -i <input.root> -o <output.h5> [options]"
  echo "Create 32x32 neural network dataset from DUNE cluster ROOT files"
  echo ""
  echo "Required arguments:"
  echo "  -i|--input <file>           Input ROOT file with clusters"
  echo "  -o|--output <file>          Output HDF5 file for dataset"
  echo ""
  echo "Optional arguments:"
  echo "  --channel-map <type>        Detector type: APA, CRP, or 50L (default: APA)"
  echo "  --min-tps <N>               Minimum TPs per cluster (default: 5)"
  echo "  --max-clusters <N>          Maximum clusters to process (for testing)"
  echo "  --only-collection           Use only collection plane (X) - recommended"
  echo "  --save-samples <N>          Number of sample PNG images to save (default: 0)"
  echo "  -v|--verbose                Enable verbose output"
  echo "  -h|--help                   Print this help message"
  echo ""
  echo "Examples:"
  echo "  # Create dataset from clusters, only collection plane, save 10 samples"
  echo "  $0 -i clusters.root -o dataset.h5 --only-collection --save-samples 10"
  echo ""
  echo "  # Quick test with 100 clusters maximum"
  echo "  $0 -i clusters.root -o test.h5 --max-clusters 100 --only-collection"
  exit 0
}

# Default values
channel_map="APA"
min_tps=5
max_clusters=""
only_collection=false
save_samples=0
verbose=false
input_file=""
output_file=""

# Parse command line options
while [[ $# -gt 0 ]]; do
  case "$1" in
    -i|--input) input_file="$2"; shift 2;;
    -o|--output) output_file="$2"; shift 2;;
    --channel-map) channel_map="$2"; shift 2;;
    --min-tps) min_tps="$2"; shift 2;;
    --max-clusters) max_clusters="$2"; shift 2;;
    --only-collection) only_collection=true; shift;;
    --save-samples) save_samples="$2"; shift 2;;
    -v|--verbose) verbose=true; shift;;
    -h|--help) print_help;;
    *) echo "Unknown option: $1"; print_help;;
  esac
done

# Validate required arguments
if [ -z "$input_file" ]; then
  echo "Error: Input file is required (-i|--input)"
  echo ""
  print_help
fi

if [ -z "$output_file" ]; then
  echo "Error: Output file is required (-o|--output)"
  echo ""
  print_help
fi

if [ ! -f "$input_file" ]; then
  echo "Error: Input file not found: $input_file"
  exit 1
fi

# Check if Python script exists
PYTHON_SCRIPT="$HOME_DIR/python/create_nn_dataset.py"
if [ ! -f "$PYTHON_SCRIPT" ]; then
  echo "Error: Python script not found: $PYTHON_SCRIPT"
  exit 1
fi

# Build command
CMD="python $PYTHON_SCRIPT"
CMD="$CMD --input $input_file"
CMD="$CMD --output $output_file"
CMD="$CMD --detector $channel_map"
CMD="$CMD --min-tps $min_tps"

if [ -n "$max_clusters" ]; then
  CMD="$CMD --max-clusters $max_clusters"
fi

if [ "$only_collection" = true ]; then
  CMD="$CMD --only-collection"
fi

if [ "$save_samples" -gt 0 ]; then
  CMD="$CMD --save-samples $save_samples"
fi

# Print configuration
echo "============================================================"
echo "Creating Neural Network Dataset"
echo "============================================================"
echo "Input file:      $input_file"
echo "Output file:     $output_file"
echo "Channel map:     $channel_map"
echo "Min TPs:         $min_tps"
if [ -n "$max_clusters" ]; then
  echo "Max clusters:    $max_clusters (testing mode)"
fi
if [ "$only_collection" = true ]; then
  echo "Planes:          Collection plane only (X)"
else
  echo "Planes:          All planes (U, V, X)"
fi
if [ "$save_samples" -gt 0 ]; then
  echo "Save samples:    $save_samples PNG images"
fi
echo "============================================================"
echo ""

# Run the command
echo "Running: $CMD"
echo ""
$CMD

exit_code=$?

if [ $exit_code -eq 0 ]; then
  echo ""
  echo "============================================================"
  echo "Dataset creation completed successfully!"
  echo "Output: $output_file"
  echo "============================================================"
else
  echo ""
  echo "============================================================"
  echo "Error: Dataset creation failed with exit code $exit_code"
  echo "============================================================"
fi

exit $exit_code
