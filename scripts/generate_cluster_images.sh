#!/bin/bash
#
# Generate numpy array images for all clusters in a cluster folder
# Creates 16x128 numpy arrays (channels x time) ready for NN input
#
# Usage: ./generate_cluster_images.sh <cluster_folder> [draw_mode]
#    OR: ./generate_cluster_images.sh --json <json_config>
#
# Examples:
#   ./generate_cluster_images.sh data/prod_es/clusters_es_valid_bg_tick3_ch2_min2_tot3_e2p0
#   ./generate_cluster_images.sh --json json/es_valid.json

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"

# Parse arguments
USE_JSON=false
JSON_FILE=""
CLUSTER_FOLDER=""
DRAW_MODE="pentagon"

if [[ $# -eq 0 ]]; then
    echo "Usage: $0 <cluster_folder> [draw_mode]"
    echo "   OR: $0 --json <json_config> [draw_mode]"
    echo ""
    echo "Arguments:"
    echo "  cluster_folder   Path to folder containing clusters_*.root files"
    echo "  --json FILE      Use JSON config to auto-detect clusters folder"
    echo "  draw_mode        Optional: pentagon (default), triangle, or rectangle"
    echo ""
    echo "Examples:"
    echo "  $0 data/prod_es/clusters_es_valid_bg_tick3_ch2_min2_tot3_e2p0"
    echo "  $0 --json json/es_valid.json"
    echo "  $0 --json json/es_valid.json triangle"
    exit 1
fi

# Check if using JSON mode
if [[ "$1" == "--json" ]] || [[ "$1" == "-j" ]]; then
    if [[ $# -lt 2 ]]; then
        echo "Error: --json requires a JSON file argument"
        exit 1
    fi
    USE_JSON=true
    JSON_FILE="$2"
    DRAW_MODE="${3:-pentagon}"
    
    # Convert to absolute path if relative
    if [[ ! "$JSON_FILE" = /* ]]; then
        JSON_FILE="${ROOT_DIR}/${JSON_FILE}"
    fi
    
    if [[ ! -f "$JSON_FILE" ]]; then
        echo "Error: JSON file not found: $JSON_FILE"
        exit 1
    fi
else
    # Directory mode
    CLUSTER_FOLDER="$1"
    DRAW_MODE="${2:-pentagon}"
    
    # Convert to absolute path if relative
    if [[ ! "$CLUSTER_FOLDER" = /* ]]; then
        CLUSTER_FOLDER="${ROOT_DIR}/${CLUSTER_FOLDER}"
    fi
    
    if [[ ! -d "$CLUSTER_FOLDER" ]]; then
        echo "Error: Cluster folder not found: $CLUSTER_FOLDER"
        exit 1
    fi
    
    # Find all clusters_*.root files
    CLUSTER_FILES=("${CLUSTER_FOLDER}"/clusters_*.root)
    
    if [[ ${#CLUSTER_FILES[@]} -eq 0 ]] || [[ ! -f "${CLUSTER_FILES[0]}" ]]; then
        echo "Error: No clusters_*.root files found in $CLUSTER_FOLDER"
        exit 1
    fi
fi

echo "=========================================="
echo "  Batch Cluster Array Generation"
echo "  (16x128 numpy arrays for NN input)"
echo "=========================================="
if [[ "$USE_JSON" == true ]]; then
    echo "JSON config: $JSON_FILE"
    echo "Mode: Auto-detect clusters folder from JSON"
else
    echo "Cluster folder: $CLUSTER_FOLDER"
    echo "Found ${#CLUSTER_FILES[@]} cluster file(s)"
fi
echo "Draw mode: $DRAW_MODE"
echo ""

# Run the Python script
cd "$ROOT_DIR"
if [[ "$USE_JSON" == true ]]; then
    python3 "${SCRIPT_DIR}/generate_cluster_arrays.py" \
        --json "$JSON_FILE" \
        --draw-mode "$DRAW_MODE"
else
    python3 "${SCRIPT_DIR}/generate_cluster_arrays.py" \
        "${CLUSTER_FILES[@]}" \
        --output-dir "$CLUSTER_FOLDER" \
        --draw-mode "$DRAW_MODE" \
        --root-dir "$ROOT_DIR"
fi

echo ""
echo "Done! Numpy arrays (.npy files) saved to: $CLUSTER_FOLDER"
