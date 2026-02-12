#!/bin/bash

export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export HOME_DIR=$(dirname $SCRIPTS_DIR)
source $HOME_DIR/scripts/init.sh

first=$1
last=$2

# for cat in $(seq $first $last); do
#     echo "Submitting cat${cat} jobs..."
#     ${SCRIPTS_DIR}/submit_cats.sh -w "${cat}" 
#     echo ""
#     sleep 3;
# done

for cat in $(seq $first $last); do
    echo "Submitting cat$(printf "%06d" $cat) jobs..."
    ${SCRIPTS_DIR}/sequence.sh -j "/afs/cern.ch/work/e/evilla/private/dune/online-pointing-utils/json/cats/cat_$(printf "%06d" $cat).json" --home-dir /afs/cern.ch/work/e/evilla/private/dune/online-pointing-utils --no-compile --all
    echo ""
    # sleep 3;
done

