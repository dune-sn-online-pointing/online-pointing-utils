#!/bin/bash

# Initialize env variables similar to cluster_to_root.sh
export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPTS_DIR/init.sh

current_dir=$(pwd)

print_help() {
	echo "************************************************************************************************"
	echo "Usage: $0 [-i input_root_file] [--no-compile] [--clean-compile]"
	echo "Options:"
	echo "  -i|--input-file <file>   Input ROOT file (default: data/cleanES60.root)"
	echo "  --no-compile             Do not recompile"
	echo "  --clean-compile          Clean and recompile"
	echo "  -h|--help                Print this help message"
	echo "************************************************************************************************"
	exit 0
}

INPUT_FILE=""
cleanCompile=false
noCompile=false

while [[ $# -gt 0 ]]; do
	case "$1" in
		-i|--input-file) INPUT_FILE="$2"; shift 2 ;;
		--no-compile)    noCompile=true; shift ;;
		--clean-compile) cleanCompile=true; shift ;;
		-h|--help)       print_help ;;
		*)               shift ;;
	esac
done

if [ -z "$INPUT_FILE" ]; then
    echo " Please specify the input ROOT file using -i or --input-file. Default is data/cleanES60.root"
    print_help
    exit
fi

################################################
# Compile if requested
compile_command="$SCRIPTS_DIR/compile.sh -p $HOME_DIR --no-compile $noCompile --clean-compile $cleanCompile"
echo "Compiling the code with the following command:"
echo $compile_command
. $compile_command || exit

################################################
# Run the app
command_to_run="$BUILD_DIR/src/app/plot_avg_times -i $INPUT_FILE"
echo "Running command: ${command_to_run}"
eval $command_to_run

cd ${current_dir}
