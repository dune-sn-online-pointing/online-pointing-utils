
REPO_HOME=$(git rev-parse --show-toplevel)
# Check if argument is provided
while [[ $# -gt 0 ]]; do
    case "$1" in
        --cut)
            cut="$2"
            shift 2
            ;;
        
        -h|--help)
            echo "Usage: ./clusters_to_dataset.sh --cut <cut>"
            exit 0
            ;;  
        *)
            shift
            ;;
    esac
done

output_file="/afs/cern.ch/work/h/hakins/private/json/ds-mix-met-vs-all/${cut}.json"
python mix_dataset_json_creator.py --cut "$cut" --output_file "$output_file"
#python datasets_in_txt_generator 

# move to the folder, run and come back to scripts
cd ${REPO_HOME}/app/
python dataset_mixer.py --input_json "$output_file" --output_folder /afs/cern.ch/work/h/hakins/private/npy_dataset/ds-mix-mt-vs-all/
cd ${REPO_HOME}/scripts/


