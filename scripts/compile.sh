#!/bin/bash

# Script to compile the code, stopping in case of error

echo "Starting script $0"

# Initialize env variables
export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPTS_DIR/init.sh

# Initialize variables
cleanCompile=false
noCompile=false
pwd=$PWD

# Determine available CPU cores (portable across Linux/macOS)
if command -v nproc >/dev/null 2>&1; then
    nproc=$(nproc)
elif command -v sysctl >/dev/null 2>&1; then
    nproc=$(sysctl -n hw.ncpu)
elif command -v getconf >/dev/null 2>&1; then
    nproc=$(getconf _NPROCESSORS_ONLN)
else
    nproc=4
fi

# Default build parallelism policy:
# - If available cores are 1..4: use 1
# - If available cores are >=5: use (cores-2), capped at 4
if [ "$nproc" -le 4 ]; then
    nproc_to_use=1
else
    nproc_to_use=$((nproc/2))
    if [ "$nproc_to_use" -gt 16 ]; then
        nproc_to_use=8
    fi
fi

# Safety clamp
if [ "$nproc_to_use" -lt 1 ]; then
    nproc_to_use=1
fi

# Function to print help message
print_help() {
    echo "*****************************************************************************"
    echo "Usage: ./compile.sh [-p <home_path>] [--clean-compile <true, false>] [--no-compile <true, false>] [-h]"
    echo "  -p | --home-path      path to the code home directory (/your/path/tof-reco, no / at the end). There is no default."
    echo "  -n | --no-compile          do not recompile the code. Default is to recompile the code"
    echo "  -j | --nproc            number of processors to use for compilation. Default is nproc-2"
    echo "  -c | --clean-compile       clean and recompile the code. Default is to recompile the code without cleaning"
    echo "  -h | --help           print this help message"
    echo "*****************************************************************************"
    exit 0
}

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
    -p|--home-path)         HOME_DIR="$2"; echo " Got home path as argument";    shift 2 ;;
    -n|--no-compile)
        if [[ "${2:-}" == "true" || "${2:-}" == "false" ]]; then
            noCompile="$2"; shift 2
        else
            noCompile=true; shift
        fi
        ;;
    -j|--nproc)
        nproc_to_use="$2"; shift 2
        if [ "$nproc_to_use" -lt 1 ]; then nproc_to_use=1; fi
        ;;
    -c|--clean-compile)
        if [[ "${2:-}" == "true" || "${2:-}" == "false" ]]; then
            cleanCompile="$2"; shift 2
        else
            cleanCompile=true; shift
        fi
        ;;
    -h|--help)              print_help ;;
    *)                      shift ;;
    esac
done


# if HOME_DIR is empty, print help message and exit
if [ -z "$HOME_DIR" ]; then
    echo "  Error: HOME_DIR is empty, meaning that you have not run from the repo and not given it as argument to the script."
    echo "  Please provide the path to the code home directory."
    print_help
fi

# if no compile, close the script
if [ "$noCompile" == true ]; then
    echo "  No compilation was requested. Stopping execution of this script."
    echo ""
else
    echo "  Compiling the code, these steps need to be run from build/"
    mkdir -p $BUILD_DIR || exit # in case it does not exist
    cd $BUILD_DIR || exit

    if [ "$cleanCompile" == true ]; then  
        echo "  Cleaning build folder..."
        rm -rfv $BUILD_DIR/*
        echo "  Done!"
    fi

    cmake ..
    # check if cmake was successful
    if [ $? -ne 0 ]
    then
        echo -e "  CMake failed. Stopping execution.\n"
        cd $pwd # go back to the original directory
        exit 1
    fi

    echo "Using $nproc_to_use processors for compilation"
    make -j $nproc_to_use
    # check if make was successful
    if [ $? -ne 0 ]
    then
        echo -e "  Make failed. Stopping execution.\n"
        cd $pwd # go back to the original directory
        exit 1
    else
        echo -e "  Compilation finished successfully!\n"
    fi

    cd $pwd # go back to the original directory
fi