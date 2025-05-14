#!/bin/bash

if [[ $INIT_DONE == true ]]; then
    echo "Environment is already initialized, not running init.sh again"
else

    echo ""
    echo "init.sh script started"

    export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    export HOME_DIR=$(dirname $SCRIPTS_DIR)
    echo " Repository home is $HOME_DIR"

    # In lxplus, nothing special should be needed 
    # In fnal, we need to be in a container and set up root

    if [[ $(hostname) == *"lxplus"* ]]; then
        echo " In lxplus, no need to source an environment or packages."
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
