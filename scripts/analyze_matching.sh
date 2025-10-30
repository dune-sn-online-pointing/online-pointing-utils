#!/bin/bash

# Script to run analyze_matching app on matched cluster files
# This analyzes the quality and efficiency of plane matching

REPO_HOME=$(git rev-parse --show-toplevel)
INPUT_JSON="${REPO_HOME}/json/analyze_matching/test.json"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -j|--json)
            INPUT_JSON="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: ./analyze_matching.sh [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -j, --json FILE    Path to JSON configuration file"
            echo "                     Default: json/analyze_matching/test.json"
            echo "  -h, --help         Show this help message"
            echo ""
            echo "JSON Format:"
            echo "{"
            echo "    \"matched_clusters_file\": \"path/to/matched_clusters.root\""
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
echo "DUNE Online Pointing - Analyze Matching"
echo "========================================="
echo "Repository: ${REPO_HOME}"
echo "JSON Config: ${INPUT_JSON}"
echo ""

# Check if JSON file exists
if [ ! -f "${INPUT_JSON}" ]; then
    echo "Error: JSON file not found: ${INPUT_JSON}"
    exit 1
fi

# Compile if needed
echo "Checking build status..."
cd "${REPO_HOME}/build" || exit 1

if [ ! -f "src/app/analyze_matching" ]; then
    echo "analyze_matching executable not found, building..."
    cmake .. && make analyze_matching -j$(nproc)
    if [ $? -ne 0 ]; then
        echo "Error: Build failed"
        exit 1
    fi
fi

echo ""
echo "Running analyze_matching..."
echo "========================================="

"${REPO_HOME}/build/src/app/analyze_matching" -j "${INPUT_JSON}"

EXIT_CODE=$?

if [ ${EXIT_CODE} -eq 0 ]; then
    echo ""
    echo "========================================="
    echo "Analysis completed successfully!"
    echo "========================================="
else
    echo ""
    echo "========================================="
    echo "Error: analyze_matching failed with exit code ${EXIT_CODE}"
    echo "========================================="
fi

exit ${EXIT_CODE}
