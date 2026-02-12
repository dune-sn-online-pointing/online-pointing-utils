#!/bin/bash

# On worker nodes, INIT_DONE might be inherited but environment is not
# Always reinitialize to ensure LCG is properly loaded
if [[ $INIT_DONE == true ]] && [[ -n "${ONLINE_POINTING_UTILS_LCG_VIEW:-}" ]]; then
    echo "Environment is already initialized, not running init.sh again."
    echo "If it is not loaded properly, run export INIT_DONE=false and rerun init.sh"
else

    echo ""
    echo "init.sh script started"

    export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    export HOME_DIR=$(dirname $SCRIPTS_DIR)
    echo " Repository home is $HOME_DIR"
    
    # Set up parameters directory
    export PARAMETERS_DIR="${HOME_DIR}/parameters"
    echo " Parameters directory is $PARAMETERS_DIR"

    # In lxplus or worker nodes, set up a full ROOT environment from LCG
    # In fnal, we need to be in a container and set up root
    
    # Check if we're at CERN (lxplus or worker nodes like b9pgpun, b9g47n, etc.)
    if [[ $(hostname) == *"lxplus"* ]] || [[ $(hostname -f) == *".cern.ch" ]]; then
        LCG_VIEW_PATH=/cvmfs/sft.cern.ch/lcg/views/LCG_104/x86_64-el9-gcc11-opt/setup.sh
        if [[ -z ${ONLINE_POINTING_UTILS_LCG_VIEW:-} ]]; then
            if [[ -f ${LCG_VIEW_PATH} ]]; then
                echo " Setting up environment for lxplus using ${LCG_VIEW_PATH}"
                source "${LCG_VIEW_PATH}"
                export ONLINE_POINTING_UTILS_LCG_VIEW=${LCG_VIEW_PATH}
            else
                echo " WARNING: Expected LCG view at ${LCG_VIEW_PATH} not found. ROOT packages may be incomplete."
            fi
        else
            echo " Environment already sourced from ${ONLINE_POINTING_UTILS_LCG_VIEW}, skipping."
        fi
    elif [[ $(hostname) == *"fnal"* ]]; then
        if grep -q "Scientific Linux release 7" /etc/redhat-release 2>/dev/null; then
            echo " Setting up environment for fnal cluster, using ups products. If this fails, it means you're not in a slf7 container."
            # Set up the basic environment
            source /cvmfs/dune.opensciencegrid.org/products/dune/setup_dune.sh
            echo " Setting up necessary packages..."
            # versions are chosen for compatibility with the pyenv
            setup root v6_26_06a -q e20:p3913:prof
            setup nlohmann_json v3_10_4_1 -q e26:prof
            setup cmake  v3_27_4
            setup valgrind v3_10_1
            setup gcc v9_3_0
            echo " Done!"
        else
            echo " WARNING: not running in slf7, can't set up dune products and commands, can't compile."
        fi
    fi

    export JSON_DIR=$HOME_DIR"/json/"
    export PYTHON_DIR=$HOME_DIR"/python/"
    export SRC_DIR=$HOME_DIR"/src/"
    export BUILD_DIR=$HOME_DIR"/build/"
    export PARAMETERS_DIR=$HOME_DIR"/parameters/"

    # activate the python environment
    if [[ -d $PYTHON_DIR/online-pointing-env ]]; then
        echo " Activating python environment"
        source $PYTHON_DIR/online-pointing-env/bin/activate
    else
        echo " WARNING: python environment not found, not activating."
    fi

    export INIT_DONE=true

    echo "init.sh finished"
    echo ""

fi
