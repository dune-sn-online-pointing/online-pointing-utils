#!/bin/bash
echo "*****************************************************************************"
echo "Running"

#source /cvmfs/sft.cern.ch/lcg/views/LCG_104cuda/x86_64-el9-gcc11-opt/setup.sh
#Before running this script, change the ADC Cut counts in each of the 3 cluster to root json files
cut=20000

setup
cd /afs/cern.ch/work/h/hakins/private/online-pointing-utils/scripts
#Main track dataset
echo "Main track cluster to root"
./cluster_to_root.sh --input_file /afs/cern.ch/work/h/hakins/private/json/cluster_to_root/main_track.json
echo "Main track cluster to dataset"
./clusters_to_dataset.sh 0
#blips
echo "Blips cluster to root"
./cluster_to_root.sh --input_file /afs/cern.ch/work/h/hakins/private/json/cluster_to_root/blips.json
echo "Blips cluster to dataset"
./clusters_to_dataset.sh 1
#bkg
echo "BKG cluster to root"
./cluster_to_root.sh --input_file /afs/cern.ch/work/h/hakins/private/json/cluster_to_root/bkg.json
echo "BKG cluster to dataset"
./clusters_to_dataset.sh 2

#mix datasets
echo "Mixing Dataset"
./mix_datasets.sh


# This script is used to run the pipeline
INPUT_JSON=/afs/cern.ch/work/h/hakins/private/ml_for_pointing/json/mt_identification/basic-hp_identification.json
OUTPUT_FOLDER=/eos/user/h/hakins/dune/ML/mt_identifier/ds-mix-mt-vs-all/{cut}/
  if [ ! -d "$output_folder" ]; then
        mkdir -p "$output_folder"
    fi
# Parse command-line arguments 
while [[ $# -gt 0 ]]; do
    case "$1" in
        -i|--input_json)
            INPUT_JSON="$2"
            shift 2
            ;;
        -o|--output_folder)
            OUTPUT_FOLDER="$2"
            shift 2
            ;;
        -h|--help)
            print_help
            ;;
        *)
            shift
            ;;
    esac
done

REPO_HOME=$(git rev-parse --show-toplevel)
export PYTHONPATH=$PYTHONPATH:$REPO_HOME/custom_python_libs/lib/python3.9/site-packages

cd /afs/cern.ch/work/h/hakins/private/ml_for_pointing/mt_identifier
echo "Starting ML Script"
python main.py --input_json $INPUT_JSON --output_folder $OUTPUT_FOLDER
cd ../scripts
