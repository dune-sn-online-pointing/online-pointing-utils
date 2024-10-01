#!/bin/bash

REPO_HOME=$(git rev-parse --show-toplevel) # does not have / at the end

# Initialize variables
input_folder=""
output_folder=""

print_help() {
    echo "*****************************************************************************"
    echo "Usage: ./printMidasFile.sh -f <first_run> [-l <last_run>] -s <settings_file> [-n <nEventToPrint>] [--offset <offset>] [-h]"
    echo "Options:"
    # echo "  -j, --settings_file <settings_file>   Settings file""
    echo " --cut <cut>                           Cut value"
    echo " --type <type>                         Type of the dataset, either 'main_track', 'blip', 'bkg', 'benchmark'"
    echo "  -h, --help                           Print this help message"
    echo "*****************************************************************************"
    exit 0
}

# Check if argument is provided
while [[ $# -gt 0 ]]; do
    case "$1" in
        --output-folder) output_folder="$2"; shift 2 ;;
        --cut)          cut="$2"; shift 2 ;;
        --type)         type="$2"; shift 2 ;;
        -h|--help)      print_help ;;
        *)              shift ;;
    esac
done

output_folder=${output_folder}/${type}/${cut}

# Create json file
output_file="/afs/cern.ch/work/h/hakins/private/json/npy/${type}/ctds_${type}_${cut}.json"
echo "creating json"
cd json_creators
python cluster_to_dataset_json_creator.py --cut "$cut" --type "$type" --output_file "$output_file"
input_json_file="$output_file"

# Execute the selected Python script
REPO_HOME=$(git rev-parse --show-toplevel)

# move to the folder, run and come back to scripts
cd ${REPO_HOME}/app/

# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_bkg.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-cc-bkg-truth/adc_80000/bkg/X/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_blips.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-cc_lab/adc_80000/blips/X/
# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_main_tracks.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/es-cc_lab/adc_80000/main_tracks/X/

python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_es-lab_volume.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/superimposed_files/es-lab_volume/
python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_cc-lab_volume.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/superimposed_files/cc-lab_volume/

# python clusters_to_dataset.py --input_json ${REPO_HOME}/json/ctds/ctds_main_tracks_direction_multiplane_clusters.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/ctds_main_tracks_direction_multiplane_clusters-3d/



cd ${REPO_HOME}/scripts/

