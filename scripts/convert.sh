#!/bin/bash
# modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-20 at 00:37:12 UTC
# modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-20 at 00:55:00 UTC (pass -j/-v/-d to convert_tpstream)
# modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-20 at 01:09:38 UTC (pass -f override)
#
# convert.sh - wrapper around the convert_tpstream app (by analogy with
# backtrack.sh). Reads the JSON settings, finds the sample's *_tpstream.root
# files, and runs convert_tpstream on each to produce the flat *_tps.root trees.
# convert_tpstream itself takes a single input file and writes <name>_tps.root
# to the current directory, so this script does the looping and cd's into the
# output folder.

set -e
export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
if [[ -f "${SCRIPTS_DIR}/init.sh" ]]; then
    source ${SCRIPTS_DIR}/init.sh
else
    if [[ -n "${HOME_DIR:-}" ]]; then
        source ${HOME_DIR}/scripts/init.sh
    else
        echo "ERROR: Cannot find init.sh and HOME_DIR not set"
        exit 1
    fi
fi

print_help(){
  echo "Usage: $0 -j <json> [--no-compile] [--clean-compile] [-o <dir>] [--max-files <n>]"
  echo "Options:"
  echo "  -j|--json <json>            Json file containing settings."
  echo "  --no-compile                Do not recompile the code"
  echo "  --clean-compile             Clean and recompile the code"
  echo "  -o|--output-folder <dir>    Output folder for *_tps.root (default: json sig_folder)."
  echo "  --max-files <n>             Maximum number of files to process (default: json max_files)."
  echo "  -f|--override               Overwrite existing output (default: skip files that exist)."
  echo "  -v|--verbose                Enable verbose mode."
  echo "  -d|--debug                  Enable debug mode."
  echo "  -h|--help                   Print this help message."
  exit 0
}

cleanCompile=false
noCompile=false
override=false
verbose=false
debug=false
max_files=""
output_folder=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    -j|--json) settingsFile="$2"; shift 2;;
    -o|--output-folder) output_folder="$2"; shift 2;;
    --max-files) max_files="$2"; shift 2;;
    --skip-files) shift 2;;   # accepted for interface parity, unused here
    --no-compile) noCompile=true; shift;;
    --clean-compile) cleanCompile=true; shift;;
    -f|--override)
      if [[ $2 == "true" || $2 == "false" ]]; then override=$2; shift 2; else override=true; shift; fi ;;
    -v|--verbose)
      if [[ $2 == "true" || $2 == "false" ]]; then verbose=$2; shift 2; else verbose=true; shift; fi ;;
    -d|--debug)
      if [[ $2 == "true" || $2 == "false" ]]; then debug=$2; shift 2; else debug=true; shift; fi ;;
    -h|--help) print_help;;
    *) shift;;
  esac
done

if [ -z "$settingsFile" ]; then
    echo " Please specify the settings file, using flag -j <settings_file>. Stopping execution."
    print_help
fi

find_settings_command="$SCRIPTS_DIR/findSettings.sh -j $settingsFile"
echo " Looking for json settings using command: $find_settings_command"
settingsFile=$($find_settings_command | tail -n 1)
echo -e "Settings file found, full path is: $settingsFile \n"

################################################
# Compile the code if requested
compile_command="$SCRIPTS_DIR/compile.sh -p $HOME_DIR --no-compile $noCompile --clean-compile $cleanCompile"
echo "Compiling the code with the following command:"
echo $compile_command
. $compile_command || exit

################################################
# Read the fields we need from the JSON (python3 is available after init.sh).
read_json(){ python3 -c "import json,sys; print(json.load(open('$settingsFile')).get('$1',''))"; }

tpstream_folder=$(read_json tpstream_folder)
file_pattern=$(read_json file_pattern)
[ -z "$file_pattern" ] && file_pattern="*_tpstream.root"
[ -z "$output_folder" ] && output_folder=$(read_json sig_folder)
[ -z "$output_folder" ] && output_folder=$(read_json outputFolder)
[ -z "$max_files" ] && max_files=$(read_json max_files)

if [ -z "$tpstream_folder" ]; then
    echo "ERROR: 'tpstream_folder' not set in $settingsFile"; exit 1
fi
mkdir -p "$output_folder"

convert_bin="$BUILD_DIR/src/app/convert_tpstream"
if [ ! -x "$convert_bin" ]; then
    echo "ERROR: convert_tpstream not found at $convert_bin (build first)."; exit 1
fi

echo "convert: tpstream_folder=$tpstream_folder  pattern=$file_pattern"
echo "convert: output_folder=$output_folder  max_files=${max_files:-all}"

################################################
# convert_tpstream reads maxcount/skipevent (and flags) from the json config.
cvt_flags="-j $settingsFile"
[ "$verbose" = true ]  && cvt_flags="$cvt_flags -v"
[ "$debug" = true ]    && cvt_flags="$cvt_flags -d"
[ "$override" = true ] && cvt_flags="$cvt_flags -f"   # overwrite existing output

# Loop over the tpstream files, converting each into the output folder.
count=0
shopt -s nullglob
for f in "$tpstream_folder"/$file_pattern; do
    if [ -n "$max_files" ] && [ "$max_files" -ge 0 ] 2>/dev/null && [ "$count" -ge "$max_files" ]; then
        echo "Reached max_files=$max_files; stopping."
        break
    fi
    echo "  [$((count+1))] $f"
    ( cd "$output_folder" && "$convert_bin" "$f" $cvt_flags )
    count=$((count+1))
done

echo "convert_tpstream processed $count file(s) into $output_folder"
