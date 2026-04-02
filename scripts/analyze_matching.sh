#!/bin/bash

# Run analyze_matching app on matched cluster files.
# Accepts either:
# 1) a JSON containing "matched_clusters_file", or
# 2) a global pipeline JSON from which the matched file is inferred.

export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPTS_DIR/init.sh"

print_help() {
    echo "========================================="
    echo "Usage: $0 -j <json_settings.json>"
    echo ""
    echo "Options:"
    echo "  -j, --json FILE    JSON settings file"
    echo "  -h, --help         Show this help message"
    echo ""
    echo "Accepted JSON formats:"
    echo "  1) { \"matched_clusters_file\": \".../file_matched.root\" }"
    echo "  2) Global pipeline JSON (infer matched file from folder settings)"
    echo "========================================="
    exit 0
}

settingsFile=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        -j|--json)
            settingsFile="$2"
            shift 2
            ;;
        -h|--help)
            print_help
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

if [ -z "$settingsFile" ]; then
    echo "Please specify settings with -j <settings_file>."
    print_help
fi

find_settings_command="$SCRIPTS_DIR/findSettings.sh -j $settingsFile"
echo "Looking for json settings using command: $find_settings_command"
settingsFile=$($find_settings_command | tail -n 1)
if [ ! -f "$settingsFile" ]; then
    echo "Error: JSON file not found: $settingsFile"
    exit 1
fi

echo "========================================="
echo "DUNE Online Pointing - Analyze Matching"
echo "========================================="
echo "Repository: ${HOME_DIR}"
echo "Settings JSON: ${settingsFile}"
echo ""

MATCHED_FILE=$(
python3 - "$settingsFile" "$HOME_DIR" <<'PY'
import glob
import json
import os
import sys

settings = os.path.abspath(sys.argv[1])
repo_home = os.path.abspath(sys.argv[2])
json_dir = os.path.dirname(settings)

with open(settings, "r", encoding="utf-8") as f:
    j = json.load(f)

def sanitize(value):
    if isinstance(value, float):
        s = f"{value:.6f}"
    else:
        s = str(value)
    if "." in s:
        left, right = s.split(".", 1)
        if len(right) > 1:
            s = f"{left}.{right[0]}"
    return s.replace(".", "p")

def resolve_existing_path(p):
    if os.path.isabs(p):
        return p
    candidates = [
        os.path.join(json_dir, p),
        os.path.join(repo_home, p),
        os.path.abspath(p),
    ]
    for c in candidates:
        if os.path.exists(c):
            return os.path.abspath(c)
    return os.path.abspath(os.path.join(repo_home, p))

def resolve_folder_path(p):
    if os.path.isabs(p):
        return p.rstrip("/")
    return os.path.abspath(os.path.join(repo_home, p)).rstrip("/")

matched_file = j.get("matched_clusters_file")
if isinstance(matched_file, str) and matched_file.strip():
    mf = resolve_existing_path(matched_file.strip())
    if not os.path.isfile(mf):
        print(f"Configured matched_clusters_file does not exist: {mf}", file=sys.stderr)
        sys.exit(2)
    print(mf)
    sys.exit(0)

matched_folder = j.get("matched_clusters_folder", "")
if not matched_folder:
    outfolder = j.get("clusters_folder", "")
    if not outfolder:
        outfolder = j.get("main_folder") or j.get("signal_folder") or j.get("tpstream_folder") or "."
    outfolder = str(outfolder).rstrip("/")

    prefix = j.get("clusters_folder_prefix", j.get("products_prefix", ""))
    conditions = (
        f"tick{sanitize(j.get('tick_limit', 0))}_"
        f"ch{sanitize(j.get('channel_limit', 0))}_"
        f"min{sanitize(j.get('min_tps_to_cluster', 0))}_"
        f"tot{sanitize(j.get('tot_cut', 0))}_"
        f"e{sanitize(float(j.get('energy_cut', 0.0)))}"
    )

    if prefix:
        matched_name = f"{prefix}_matched_clusters_{conditions}"
    else:
        matched_name = f"matched_clusters_{conditions}"
    matched_folder = os.path.join(outfolder, matched_name)

matched_folder = resolve_folder_path(str(matched_folder))
if not os.path.isdir(matched_folder):
    print(f"Matched clusters folder not found: {matched_folder}", file=sys.stderr)
    sys.exit(3)

candidates = sorted(glob.glob(os.path.join(matched_folder, "*_matched.root")))
if not candidates:
    print(f"No *_matched.root files found in: {matched_folder}", file=sys.stderr)
    sys.exit(4)

print(os.path.abspath(candidates[0]))
PY
)

if [ $? -ne 0 ] || [ -z "$MATCHED_FILE" ]; then
    echo "Error: Failed to infer matched_clusters_file from ${settingsFile}"
    exit 1
fi

TMP_JSON=$(mktemp /tmp/analyze_matching_config_XXXXXX.json)
trap 'rm -f "$TMP_JSON"' EXIT

cat > "$TMP_JSON" <<EOF
{
  "matched_clusters_file": "${MATCHED_FILE}"
}
EOF

echo "Matched clusters file: ${MATCHED_FILE}"
echo ""

echo "Checking build status..."
cd "${BUILD_DIR}" || exit 1

if [ ! -f "src/app/analyze_matching" ]; then
    echo "analyze_matching executable not found, building..."
    cmake .. && make analyze_matching -j"$(nproc)"
    if [ $? -ne 0 ]; then
        echo "Error: Build failed"
        exit 1
    fi
fi

echo ""
echo "Running analyze_matching..."
echo "========================================="

"${BUILD_DIR}/src/app/analyze_matching" -j "$TMP_JSON"
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
