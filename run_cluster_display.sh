#!/bin/bash
# Script to run cluster_display app
# Usage: ./run_cluster_display.sh <clusters_file> [mode]
#
# Arguments:
#   clusters_file: Path to the clusters ROOT file (required, must contain 'clusters' in name)
#   mode: Display mode - 'clusters' (default) or 'events'
#
# Example:
#   ./run_cluster_display.sh data/cleanES60_tps_bktr10_clusters_tick3_ch2_min1_tot0_en0.root
#   ./run_cluster_display.sh data/cleanES60_tps_bktr10_clusters_tick3_ch2_min1_tot0_en0.root events

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Path to the cluster_display executable
CLUSTER_DISPLAY="${SCRIPT_DIR}/build/src/app/cluster_display"

# Check if executable exists
if [ ! -f "$CLUSTER_DISPLAY" ]; then
    echo "Error: cluster_display executable not found at $CLUSTER_DISPLAY"
    echo "Please build the project first: cd build && make cluster_display"
    exit 1
fi

# Check if clusters file is provided
if [ -z "$1" ]; then
    echo "Error: No clusters file provided"
    echo ""
    echo "Usage: $0 <clusters_file> [mode]"
    echo ""
    echo "Arguments:"
    echo "  clusters_file: Path to the clusters ROOT file (required)"
    echo "  mode: Display mode - 'clusters' (default) or 'events'"
    echo ""
    echo "Example:"
    echo "  $0 data/cleanES60_tps_bktr10_clusters_tick3_ch2_min1_tot0_en0.root"
    echo "  $0 data/cleanES60_tps_bktr10_clusters_tick3_ch2_min1_tot0_en0.root events"
    exit 1
fi

CLUSTERS_FILE="$1"
MODE="${2:-clusters}"  # Default to 'clusters' if not specified

# Check if file exists
if [ ! -f "$CLUSTERS_FILE" ]; then
    echo "Error: Clusters file not found: $CLUSTERS_FILE"
    exit 1
fi

# Check if filename contains 'clusters'
if [[ ! "$CLUSTERS_FILE" =~ clusters ]]; then
    echo "Warning: File '$CLUSTERS_FILE' does not contain 'clusters' in name."
    echo "Are you sure this is a clusters file? (press Ctrl+C to cancel, or wait 3 seconds to continue)"
    sleep 3
fi

# Run the cluster_display app
echo "Running cluster_display..."
echo "  Clusters file: $CLUSTERS_FILE"
echo "  Mode: $MODE"
echo ""

"$CLUSTER_DISPLAY" --clusters-file "$CLUSTERS_FILE" --mode "$MODE"

exit $?
