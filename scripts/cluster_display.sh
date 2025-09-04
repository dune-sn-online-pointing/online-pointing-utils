#!/usr/bin/env bash
set -euo pipefail

# Deprecated wrapper. The script has been renamed to display.sh and now mirrors cluster_to_root.sh.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "[cluster_display.sh] DEPRECATED: use scripts/display.sh instead. Forwarding..." >&2
exec "$SCRIPT_DIR/display.sh" "$@"
