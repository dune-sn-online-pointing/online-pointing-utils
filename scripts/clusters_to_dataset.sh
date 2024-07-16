#!/bin/bash

# Check if argument is provided
while [[ $# -gt 0 ]]; do
    case "$1" in
        --cut)
            cut="$2"
            shift 2
            ;;

        --type)
            type="$2"
            shift 2
            ;;
        
        -h|--help)
            echo "Usage: ./clusters_to_dataset.sh --cut <cut> --type <type> Options: main_track, blip, bkg, benchmark"
            exit 0
            ;;  
        *)
            shift
            ;;
    esac
done

output_folder="/afs/cern.ch/work/h/hakins/private/npy_dataset/es-cc-bkg-truth/"

if [ "$type" == "bkg" ]; then
    output_folder="/afs/cern.ch/work/h/hakins/private/npy_dataset/es-cc-bkg-truth/${cut}"
elif [ "$type" == "benchmark" ]; then
    output_folder="/afs/cern.ch/work/h/hakins/private/npy_dataset/benchmark/${cut}"
else    
    output_folder="/afs/cern.ch/work/h/hakins/private/npy_dataset/es-cc_lab/${type}/${cut}"
fi


# Create json file
output_file="/afs/cern.ch/work/h/hakins/private/json/npy/${type}/ctds_${type}_${cut}.json"
echo "creating json"
cd json_creators
python cluster_to_dataset_json_creator.py --cut "$cut" --type "$type" --output_file "$output_file"
input_json_file="$output_file"

# Execute the selected Python script
REPO_HOME=$(git rev-parse --show-toplevel)
cd "${REPO_HOME}/app/" || exit
python clusters_to_dataset.py --input_json "$input_json_file" --output_folder "$output_folder"
cd "${REPO_HOME}/scripts/" || exit
