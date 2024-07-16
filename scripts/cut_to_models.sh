#!/bin/bash
echo "*****************************************************************************"
echo "Running"
source /cvmfs/dunedaq.opensciencegrid.org/setup_dunedaq.sh 
setup_dbt latest dbt-setup-release fddaq-v4.2.0
export PYTHONPATH=/afs/cern.ch/user/h/hakins/private/matplotlib/lib/python3.10/site-packages/:$PYTHONPATH
source /cvmfs/sft.cern.ch/lcg/views/LCG_105/x86_64-el9-gcc13-opt/setup.sh

while [[ $# -gt 0 ]]; do

    case "$1" in
        -cut| --cut)
            cut="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: ./cut_to_models.sh -cut <cut>"
            exit 0
            ;;  
        *)
            shift
            ;;
    esac
done


echo "Current cut value: $cut"
cd /afs/cern.ch/work/h/hakins/private/online-pointing-utils/scripts
#Main track dataset
echo "Main track cluster to root"
./cluster_to_root.sh --cut "$cut" --type main_track 
echo "Main track cluster to dataset"
./clusters_to_dataset.sh --cut "$cut" --type main_track
#blips
echo "Blips cluster to root"
./cluster_to_root.sh --cut "$cut" --type blip 
echo "Blips cluster to dataset"
./clusters_to_dataset.sh --cut "$cut" --type blip
#bkg
echo "BKG cluster to root"
./cluster_to_root.sh --cut "$cut" --type bkg 
echo "BKG cluster to dataset"
./clusters_to_dataset.sh --cut "$cut" --type bkg

#mix datasets
echo "Mixing Dataset"
./mix_datasets.sh --cut "$cut"

#create output folder for model if not already there
OUTPUT_FOLDER=/eos/user/h/hakins/dune/ML/mt_identifier/ds-mix-mt-vs-all/${cut}/
if [ ! -d "$OUTPUT_FOLDER" ]; then
        mkdir -p "$OUTPUT_FOLDER"
    fi
#train model with hyperopt 
echo "running run_mt_identifier"
cd /afs/cern.ch/work/h/hakins/private/ml_for_pointing/scripts
source run_mt_identifier.sh --output_folder ${OUTPUT_FOLDER} --cut "$cut"


