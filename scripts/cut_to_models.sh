#!/bin/bash
echo "*****************************************************************************"
echo "Running"
setup
while [[ $# -gt 0 ]]; do
    case "$1" in
        -cut)
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
./cluster_to_root.sh --input_file /afs/cern.ch/work/h/hakins/private/json/cluster_to_root/main_tracks/main_track_${cut}.json
echo "Main track cluster to dataset"
./clusters_to_dataset.sh --cut "$cut" --type main_track
#blips
echo "Blips cluster to root"
./cluster_to_root.sh --input_file /afs/cern.ch/work/h/hakins/private/json/cluster_to_root/blips/blips_${cut}.json
echo "Blips cluster to dataset"
./clusters_to_dataset.sh --cut "$cut" --type blip
#bkg
echo "BKG cluster to root"
./cluster_to_root.sh --input_file /afs/cern.ch/work/h/hakins/private/json/cluster_to_root/bkg/bkg_${cut}.json
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


