#!/bin/bash

set -e
export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPTS_DIR/init.sh

print_help(){
  echo "Usage: $0 <dataset.h5> [options]"
  echo "Visualize 32x32 neural network dataset"
  echo ""
  echo "Required argument:"
  echo "  dataset                     HDF5 dataset file to visualize"
  echo ""
  echo "Optional arguments:"
  echo "  -n|--n-samples <N>          Number of sample images to display (default: 16)"
  echo "  -o|--output <file>          Save visualization to file instead of displaying"
  echo "  --stats                     Show dataset statistics instead of samples"
  echo "  --no-labels                 Do not show labels on images"
  echo "  -h|--help                   Print this help message"
  echo ""
  echo "Examples:"
  echo "  # Display 16 sample images in a grid"
  echo "  $0 dataset.h5"
  echo ""
  echo "  # Save 25 samples to PNG file"
  echo "  $0 dataset.h5 -n 25 -o samples.png"
  echo ""
  echo "  # Show dataset statistics"
  echo "  $0 dataset.h5 --stats -o stats.png"
  exit 0
}

# Default values
dataset_file=""
n_samples=16
output_file=""
show_stats=false
no_labels=false

# Parse command line options
while [[ $# -gt 0 ]]; do
  case "$1" in
    -n|--n-samples) n_samples="$2"; shift 2;;
    -o|--output) output_file="$2"; shift 2;;
    --stats) show_stats=true; shift;;
    --no-labels) no_labels=true; shift;;
    -h|--help) print_help;;
    *)
      if [ -z "$dataset_file" ]; then
        dataset_file="$1"
        shift
      else
        echo "Unknown option: $1"
        print_help
      fi
      ;;
  esac
done

# Validate required arguments
if [ -z "$dataset_file" ]; then
  echo "Error: Dataset file is required"
  echo ""
  print_help
fi

if [ ! -f "$dataset_file" ]; then
  echo "Error: Dataset file not found: $dataset_file"
  exit 1
fi

# Check if Python script exists
PYTHON_SCRIPT="$HOME_DIR/python/visualize_dataset.py"
if [ ! -f "$PYTHON_SCRIPT" ]; then
  echo "Error: Python script not found: $PYTHON_SCRIPT"
  exit 1
fi

# Build command
CMD="python $PYTHON_SCRIPT $dataset_file"

if [ -n "$output_file" ]; then
  CMD="$CMD --output $output_file"
fi

if [ "$show_stats" = true ]; then
  CMD="$CMD --stats"
else
  CMD="$CMD --n-samples $n_samples"
fi

if [ "$no_labels" = true ]; then
  CMD="$CMD --no-labels"
fi

# Print configuration
echo "============================================================"
echo "Visualizing Neural Network Dataset"
echo "============================================================"
echo "Dataset file:    $dataset_file"
if [ "$show_stats" = true ]; then
  echo "Mode:            Statistics"
else
  echo "Mode:            Sample images"
  echo "Number samples:  $n_samples"
  if [ "$no_labels" = true ]; then
    echo "Show labels:     No"
  else
    echo "Show labels:     Yes"
  fi
fi
if [ -n "$output_file" ]; then
  echo "Output file:     $output_file"
else
  echo "Output:          Display on screen"
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
  echo "Visualization completed successfully!"
  if [ -n "$output_file" ]; then
    echo "Output: $output_file"
  fi
  echo "============================================================"
else
  echo ""
  echo "============================================================"
  echo "Error: Visualization failed with exit code $exit_code"
  echo "============================================================"
fi

exit $exit_code
