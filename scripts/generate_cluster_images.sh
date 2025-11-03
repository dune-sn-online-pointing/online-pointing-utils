#!/bin/bash
set -e
export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPTS_DIR/init.sh

print_help() {
    echo "Usage: $0 -j <json> [-o <output>] [-d|--draw-mode <mode>] [-f|--override] [-v|--verbose] [--skip-files <n>] [--max-files <n>]"
    echo "Options:"
    echo "  -j|--json <file>            JSON settings file (required)"
    echo "  -o|--output-dir <dir>       Output directory (overrides JSON clusters_folder)"
    echo "  -d|--draw-mode <mode>       Drawing mode: pentagon (default), triangle, or rectangle"
    echo "  -f|--override               Force reprocessing even if output batch files already exist"
    echo "  -v|--verbose                Enable verbose output"
    echo "     --skip-files <n>        Skip first N cluster files (overrides JSON)"
    echo "     --max-files <n>         Process at most N cluster files (overrides JSON)"
    echo "  -h|--help                   Print this help message"
    echo ""
    echo "Example:"
    echo "  $0 -j json/es_valid.json"
    echo "  $0 -j json/es_valid.json -d triangle -v"
    echo "  $0 -j json/es_valid.json -f  # Force reprocess all files"
    exit 0
}

settingsFile=""
output_dir=""
draw_mode="pentagon"
override=false
verbose=false
skip_files=""
max_files=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        -j|--json) settingsFile="$2"; shift 2;;
        -o|--output-dir) output_dir="$2"; shift 2;;
        -d|--draw-mode) draw_mode="$2"; shift 2;;
        --skip-files) skip_files="$2"; shift 2;;
        --max-files) max_files="$2"; shift 2;;
        -f|--override)
            if [[ $2 == "true" || $2 == "false" ]]; then
                override=$2
                shift 2
            else
                override=true
                shift
            fi
            ;;
        -v|--verbose)
            if [[ $2 == "true" || $2 == "false" ]]; then
                verbose=$2
                shift 2
            else
                verbose=true
                shift
            fi
            ;;
        -h|--help) print_help;;
        *) echo "Unknown option: $1"; print_help;;
    esac
done

# Validate required arguments
if [[ -z "$settingsFile" ]]; then
    echo "Error: JSON settings file required (-j)"
    print_help
fi

settingsFile=$($SCRIPTS_DIR/findSettings.sh -j $settingsFile | tail -n 1)

if [[ ! -f "$settingsFile" ]]; then
    echo "Error: JSON file not found: $settingsFile"
    exit 1
fi

# Build command
cmd="python3 ${SCRIPTS_DIR}/../python/generate_cluster_arrays.py --json $settingsFile --draw-mode $draw_mode"
if [[ -n "$skip_files" ]]; then
    cmd+=" --skip-files $skip_files"
fi
if [[ -n "$max_files" ]]; then
    cmd+=" --max-files $max_files"
fi

if [[ -n "$output_dir" ]]; then
    cmd+=" --output-dir $output_dir"
fi

if [ "$override" = true ]; then
    cmd+=" --override"
fi

if [ "$verbose" = true ]; then
    cmd+=" --verbose"
fi

echo "Running: $cmd"
exec $cmd
