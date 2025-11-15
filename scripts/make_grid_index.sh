#!/usr/bin/env bash
set -euo pipefail
OUTFOLDER=${1:-data}
INDEX="$OUTFOLDER/clean60_grid_index.html"
{
  echo "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Clean 60 grid reports</title>"
  echo "<style>body{font-family:sans-serif;margin:20px} h2{margin-top:1.5em} code{background:#f3f3f3;padding:2px 4px;border-radius:3px} li{margin:2px 0}</style>"
  echo "</head><body>"
  echo "<h1>Grid results: clean ES60 and CC60</h1>"
  for sample in cleanES60_tps_bktr5.root cleanCC60_tps_bktr5.root; do
    echo "<h2>$sample</h2>"
    echo "<ul>"
    # List PDFs for this sample
    for f in $(ls "$OUTFOLDER"/${sample}_clusters_tick*_ch*_min1_tot*_en0.pdf 2>/dev/null | sort); do
      base=$(basename "$f")
      echo "<li><a href=\"$base\">$base</a></li>"
    done
    echo "</ul>"
  done
  echo "</body></html>"
} > "$INDEX"
echo "$INDEX"
