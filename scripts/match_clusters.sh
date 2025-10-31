#!/bin/bash

# Script to run match_clusters app on pre-computed cluster files
# This matches clusters across U, V, X planes using geometric compatibility

REPO_HOME=$(git rev-parse --show-toplevel)
INPUT_JSON="${REPO_HOME}/json/match_clusters/test.json"
NO_COMPILE=false
MAX_FILES=""
SKIP_FILES=""
OVERRIDE=false
VERBOSE=false
DEBUG=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -j|--json)
            INPUT_JSON="$2"
            shift 2
            ;;
        --no-compile)
            NO_COMPILE=true
            shift
            ;;
        --max-files)
            MAX_FILES="$2"
            shift 2
            ;;
        --skip-files)
            SKIP_FILES="$2"
            shift 2
            ;;
        -f)
            OVERRIDE="$2"
            shift 2
            ;;
        -v)
            VERBOSE="$2"
            shift 2
            ;;
        -d)
            DEBUG="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: ./match_clusters.sh [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -j, --json FILE      Path to JSON configuration file"
            echo "                       Default: json/match_clusters/test.json"
            echo "  --no-compile         Skip compilation check"
            echo "  --max-files N        Maximum number of files to process"
            echo "  --skip-files N       Number of files to skip"
            echo "  -f true/false        Override existing files"
            echo "  -v true/false        Verbose mode"
            echo "  -d true/false        Debug mode"
            echo "  -h, --help           Show this help message"
            echo ""
            echo "JSON Format:"
            echo "{"
            echo "    \"clusters_folder\": \"path/to/clusters_folder/\","
            echo "    \"matched_clusters_folder\": \"path/to/output_folder/\","
            echo "    \"time_tolerance_ticks\": 100,"
            echo "    \"spatial_tolerance_cm\": 5.0"
            echo "}"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

echo "========================================="
echo "DUNE Online Pointing - Match Clusters"
echo "========================================="
echo "Repository: ${REPO_HOME}"
echo "JSON Config: ${INPUT_JSON}"
echo ""

# Check if JSON file exists
if [ ! -f "${INPUT_JSON}" ]; then
    echo "Error: JSON file not found: ${INPUT_JSON}"
    exit 1
fi

# Compile if needed (unless --no-compile flag is set)
if [ "$NO_COMPILE" = false ]; then
    echo "Checking build status..."
    cd "${REPO_HOME}/build" || exit 1

    if [ ! -f "src/app/match_clusters" ]; then
        echo "match_clusters executable not found, building..."
        cmake .. && make match_clusters -j$(nproc)
        if [ $? -ne 0 ]; then
            echo "Error: Build failed"
            exit 1
        fi
    fi
    cd "${REPO_HOME}"
else
    echo "Skipping compilation check (--no-compile flag set)"
fi

echo ""
echo "Running match_clusters..."
echo "========================================="

# Parse JSON to check if we need to process a folder or a single file
# Auto-generate clusters_folder if not specified (same logic as C++ code)
CLUSTERS_FOLDER=$(python3 -c "
import json
with open('${INPUT_JSON}') as f:
    j = json.load(f)
    
clusters_folder = j.get('clusters_folder', '')
if not clusters_folder:
    # Auto-generate from tpstream_folder and clustering parameters
    tpstream_folder = j.get('tpstream_folder', '.')
    if tpstream_folder.endswith('/'):
        tpstream_folder = tpstream_folder[:-1]
    
    prefix = j.get('clusters_folder_prefix', 'clusters')
    tick_limit = j.get('tick_limit', 0)
    channel_limit = j.get('channel_limit', 0)
    min_tps = j.get('min_tps_to_cluster', 0)
    tot_cut = j.get('tot_cut', 0)
    energy_cut = j.get('energy_cut', 0.0)
    
    # Format energy_cut to match C++ sanitization (keep 1 decimal, replace . with p)
    energy_str = f'{energy_cut:.1f}'.replace('.', 'p')
    
    subfolder = f'clusters_{prefix}_tick{tick_limit}_ch{channel_limit}_min{min_tps}_tot{tot_cut}_e{energy_str}'
    clusters_folder = f'{tpstream_folder}/{subfolder}'

print(clusters_folder)
" 2>/dev/null)
INPUT_FILE=$(python3 -c "import json; f=open('${INPUT_JSON}'); j=json.load(f); print(j.get('input_clusters_file', ''))" 2>/dev/null)
MATCHED_FOLDER=$(python3 -c "import json; f=open('${INPUT_JSON}'); j=json.load(f); print(j.get('matched_clusters_folder', ''))" 2>/dev/null)

# Auto-generate matched_clusters_folder if not specified
if [ -z "$MATCHED_FOLDER" ] && [ ! -z "$CLUSTERS_FOLDER" ]; then
    MATCHED_FOLDER="${CLUSTERS_FOLDER}/matched"
fi

OUTPUT_FILE=$(python3 -c "import json; f=open('${INPUT_JSON}'); j=json.load(f); print(j.get('output_file', ''))" 2>/dev/null)
JSON_MAX_FILES=$(python3 -c "import json; f=open('${INPUT_JSON}'); j=json.load(f); print(j.get('max_files', 999999))" 2>/dev/null)

# Determine max files to process
if [ ! -z "$MAX_FILES" ]; then
    ACTUAL_MAX_FILES="$MAX_FILES"
else
    ACTUAL_MAX_FILES="$JSON_MAX_FILES"
fi

if [ ! -z "$CLUSTERS_FOLDER" ] && [ -d "$CLUSTERS_FOLDER" ]; then
    # Batch mode: process all cluster files in the folder
    echo "Processing clusters from folder: ${CLUSTERS_FOLDER}"
    echo "Output will be written to: ${MATCHED_FOLDER}"
    echo "Maximum files to process: ${ACTUAL_MAX_FILES}"
    echo ""
    
    # Create output folder if it doesn't exist
    mkdir -p "${MATCHED_FOLDER}"
    
    # Find all *_clusters.root files
    CLUSTER_FILES=$(find "${CLUSTERS_FOLDER}" -name "*_clusters.root" | sort | head -n ${ACTUAL_MAX_FILES})
    FILE_COUNT=$(echo "$CLUSTER_FILES" | wc -l)
    
    echo "Found ${FILE_COUNT} cluster files to process"
    echo ""
    
    PROCESSED=0
    FAILED=0
    
    for CLUSTER_FILE in $CLUSTER_FILES; do
        BASENAME=$(basename "$CLUSTER_FILE" _clusters.root)
        OUTPUT_MATCHED="${MATCHED_FOLDER}/${BASENAME}_matched.root"
        
        echo "[$((PROCESSED+1))/${FILE_COUNT}] Processing: $(basename $CLUSTER_FILE)"
        
        # Create temporary JSON for this file
        TMP_JSON="/tmp/match_clusters_$$.json"
        python3 -c "
import json
with open('${INPUT_JSON}', 'r') as f:
    config = json.load(f)
config['input_clusters_file'] = '${CLUSTER_FILE}'
config['output_file'] = '${OUTPUT_MATCHED}'
with open('${TMP_JSON}', 'w') as f:
    json.dump(config, f, indent=4)
"
        
        # Run match_clusters
        "${REPO_HOME}/build/src/app/match_clusters" -j "${TMP_JSON}" > /dev/null 2>&1
        EXIT_CODE=$?
        
        rm -f "${TMP_JSON}"
        
        if [ ${EXIT_CODE} -eq 0 ]; then
            echo "  ✓ Success: ${OUTPUT_MATCHED}"
            PROCESSED=$((PROCESSED+1))
        else
            echo "  ✗ Failed with exit code ${EXIT_CODE}"
            FAILED=$((FAILED+1))
        fi
        echo ""
    done
    
    echo "========================================="
    echo "Batch processing complete!"
    echo "Processed: ${PROCESSED} files"
    echo "Failed: ${FAILED} files"
    echo "========================================="
    
    EXIT_CODE=0
    if [ ${FAILED} -gt 0 ]; then
        EXIT_CODE=1
    fi
    
elif [ ! -z "$INPUT_FILE" ] && [ -f "$INPUT_FILE" ]; then
    # Single file mode
    echo "Processing single file: ${INPUT_FILE}"
    "${REPO_HOME}/build/src/app/match_clusters" -j "${INPUT_JSON}"
    EXIT_CODE=$?
    
    if [ ${EXIT_CODE} -eq 0 ]; then
        echo ""
        echo "========================================="
        echo "Match clusters completed successfully!"
        echo "========================================="
    else
        echo ""
        echo "========================================="
        echo "Error: match_clusters failed with exit code ${EXIT_CODE}"
        echo "========================================="
    fi
else
    echo "Error: No valid input specified in JSON"
    echo "Please provide either 'clusters_folder' or 'input_clusters_file'"
    EXIT_CODE=1
fi

exit ${EXIT_CODE}
