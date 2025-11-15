#!/bin/bash

export SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export HOME_DIR=$(dirname $SCRIPTS_DIR)
source $HOME_DIR/scripts/init.sh

first=$1
last=$2

for cat in $(seq $first $last); do
    echo "Submitting cat${cat} jobs..."
    ${SCRIPTS_DIR}/submit_cats.sh -w "${cat}" 
    echo ""
    sleep 100;
done
