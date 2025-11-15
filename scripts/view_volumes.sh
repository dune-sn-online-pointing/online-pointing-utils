#!/bin/bash

# Script to view volume images interactively
# Usage: ./scripts/view_volumes.sh -j <json_config> [options]

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Default values
JSON_CONFIG=""
VOLUMES_FOLDER=""
VERBOSE=false
NO_TPS=false

# Function to show usage
usage() {
    cat << EOF
Usage: $0 -j <json_config> [options]

Options:
    -j, --json <file>           JSON configuration file (required)
    -v, --volumes <folder>      Volume images folder (auto-generated if not provided)
    --no-tps                    Show clusters as colored dots instead of full TP images
                                (Red=Main Track, Orange=MARLEY, Blue=Background)
    --verbose                   Enable verbose output
    -h, --help                  Show this help message

Example:
    $0 -j json/es_valid_test.json
    $0 -j json/es_valid_test.json --no-tps
    $0 -j json/es_valid_test.json -v data2/prod_es/volume_images_custom/

Notes:
    - If volumes_folder is not specified, it will be auto-generated from:
      tpstream_folder and clustering parameters (tick_tolerance, ch_tolerance, etc.)
    - The script will launch an interactive matplotlib viewer with navigation buttons
    - Use arrow keys, space, or buttons to navigate through volumes
    - Press 'q' to quit the viewer
    - --no-tps mode requires uproot package (pip install uproot)

EOF
    exit 1
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -j|--json)
            JSON_CONFIG="$2"
            shift 2
            ;;
        -v|--volumes)
            VOLUMES_FOLDER="$2"
            shift 2
            ;;
        --no-tps)
            NO_TPS=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "Error: Unknown option $1"
            usage
            ;;
    esac
done

# Check if JSON config is provided
if [ -z "$JSON_CONFIG" ]; then
    echo "Error: JSON configuration file is required"
    usage
fi

# Check if JSON file exists
if [ ! -f "$JSON_CONFIG" ]; then
    echo "Error: JSON file not found: $JSON_CONFIG"
    exit 1
fi

# If volumes_folder not provided, auto-generate it
if [ -z "$VOLUMES_FOLDER" ]; then
    echo "Auto-generating volumes_folder from JSON config..."
    
    # Use Python to read JSON and generate volumes folder path
    VOLUMES_FOLDER=$(python3 << EOF
import json
import sys

try:
    with open('$JSON_CONFIG', 'r') as f:
        config = json.load(f)
    
    # Get parameters (matching create_volumes.py logic)
    tpstream_folder = config.get('tpstream_folder', '').rstrip('/')
    prefix = config.get('clusters_folder_prefix', 'volumes')
    
    # Get clustering parameters (try both naming conventions)
    tick_tolerance = config.get('tick_tolerance', config.get('tick_limit', 3))
    ch_tolerance = config.get('ch_tolerance', config.get('channel_limit', 2))
    min_tps = config.get('min_tps_per_cluster', config.get('min_tps_to_cluster', 2))
    tot_threshold = config.get('tot_threshold', config.get('tot_cut', 3))
    energy_threshold = config.get('energy_threshold', config.get('energy_cut', 2.0))
    
    if not tpstream_folder:
        print("Error: tpstream_folder not found in JSON config", file=sys.stderr)
        sys.exit(1)
    
    # Build conditions string (matching create_volumes.py line 566)
    conditions = f"tick{tick_tolerance}_ch{ch_tolerance}_min{min_tps}_tot{tot_threshold}_e{int(energy_threshold)}p{int((energy_threshold % 1) * 10)}"
    
    # Build volumes folder path (matching create_volumes.py line 566)
    volumes_folder = f"{tpstream_folder}/volume_images_{prefix}_{conditions}"
    
    print(volumes_folder)
    
except Exception as e:
    print(f"Error parsing JSON: {e}", file=sys.stderr)
    sys.exit(1)
EOF
)
    
    if [ $? -ne 0 ] || [ -z "$VOLUMES_FOLDER" ]; then
        echo "Error: Failed to auto-generate volumes_folder"
        exit 1
    fi
    
    echo "Using volumes_folder: $VOLUMES_FOLDER"
fi

# Check if volumes folder exists
if [ ! -d "$VOLUMES_FOLDER" ]; then
    echo "Error: Volumes folder not found: $VOLUMES_FOLDER"
    echo ""
    echo "Did you run create_volumes first?"
    echo "  ./scripts/create_volumes.sh -j $JSON_CONFIG"
    exit 1
fi

# Count volume files
NUM_VOLUMES=$(find "$VOLUMES_FOLDER" -name "*.npz" -type f | wc -l)

if [ $NUM_VOLUMES -eq 0 ]; then
    echo "Error: No .npz volume files found in $VOLUMES_FOLDER"
    exit 1
fi

echo ""
echo "=============================================="
echo "Volume Image Viewer"
echo "=============================================="
echo "JSON Config:      $JSON_CONFIG"
echo "Volumes Folder:   $VOLUMES_FOLDER"
echo "Number of Volumes: $NUM_VOLUMES"
echo "Display Mode:     $([ "$NO_TPS" = true ] && echo 'Cluster Dots' || echo 'Full TPs')"
echo "=============================================="
echo ""
echo "Launching interactive viewer..."
echo "  - Use arrow keys or buttons to navigate"
echo "  - Press 'q' to quit"
if [ "$NO_TPS" = true ]; then
    echo "  - Color code: Red=Main Track, Orange=MARLEY, Blue=Background"
fi
echo ""

# Launch the interactive viewer
cd "$PROJECT_DIR"
if [ "$NO_TPS" = true ]; then
    python3 view_volumes_interactive.py "$VOLUMES_FOLDER" --no-tps
else
    python3 view_volumes_interactive.py "$VOLUMES_FOLDER"
fi

EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
    echo ""
    echo "Error: Viewer exited with code $EXIT_CODE"
    exit $EXIT_CODE
fi

echo ""
echo "Viewer closed."
