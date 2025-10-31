#!/bin/bash
# Wrapper script for volume image creation with JSON interface

set -e  # Exit on error

# Initialize environment
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

source "$REPO_ROOT/scripts/init.sh"

# Default values
VERBOSE_FLAG=""
SKIP_OVERRIDE=""
MAX_OVERRIDE=""
OVERRIDE_FLAG=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -j|--json)
            JSON_FILE="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE_FLAG="-v"
            shift
            ;;
        -f|--override)
            OVERRIDE_FLAG="-f"
            shift
            ;;
        --skip)
            SKIP_OVERRIDE="--skip $2"
            shift 2
            ;;
        --max)
            MAX_OVERRIDE="--max $2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 -j <json_file> [-v] [-f] [--skip N] [--max N]"
            exit 1
            ;;
    esac
done

# Check if JSON file was provided
if [ -z "$JSON_FILE" ]; then
    echo "Error: JSON file required (-j option)"
    echo "Usage: $0 -j <json_file> [-v] [-f] [--skip N] [--max N]"
    exit 1
fi

# Check if JSON file exists
if [ ! -f "$JSON_FILE" ]; then
    echo "Error: JSON file not found: $JSON_FILE"
    exit 1
fi

echo ""
echo "=================================================="
echo "Volume Image Creation Pipeline"
echo "=================================================="
echo "JSON config: $JSON_FILE"
echo "=================================================="
echo ""

# Run the Python script
python3 "$SCRIPT_DIR/create_volumes.py" -j "$JSON_FILE" $VERBOSE_FLAG $OVERRIDE_FLAG $SKIP_OVERRIDE $MAX_OVERRIDE

EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo ""
    echo "Volume image creation completed successfully!"
else
    echo ""
    echo "Volume image creation failed with exit code $EXIT_CODE"
fi

exit $EXIT_CODE
