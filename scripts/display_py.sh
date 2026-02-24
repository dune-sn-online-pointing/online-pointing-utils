#!/bin/bash
# Convenience wrapper for Python cluster display application

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SCRIPT="$SCRIPT_DIR/../python/ana/cluster_display.py"

# Check if Python script exists
if [[ ! -f "$PYTHON_SCRIPT" ]]; then
    echo "Error: Python script not found at $PYTHON_SCRIPT"
    exit 1
fi

# Check dependencies
python3 -c "import uproot, matplotlib, numpy" 2>/dev/null
if [[ $? -ne 0 ]]; then
    echo "Error: Missing Python dependencies. Install with:"
    echo "  pip install -r python/requirements.txt"
    exit 1
fi

# Run the Python script with all arguments passed through
python3 "$PYTHON_SCRIPT" "$@"
