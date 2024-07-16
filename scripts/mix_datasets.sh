
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
cd /afs/cern.ch/work/h/hakins/private/online-pointing-utils/scripts/json_creators
echo Calling mix_dataset_json_creator
python mix_dataset_json_creator.py --cut "$cut" --output_file "$output_file" #create json at output_file location
#python datasets_in_txt_generator 

# move to the folder, run and come back to scripts
echo calling dataset_mixer.py
cd ${REPO_HOME}/app/
echo Output File: ${output_file}
python dataset_mixer.py --input_json "$output_file" --output_folder /afs/cern.ch/work/h/hakins/private/npy_dataset/ds-mix-mt-vs-all/
cd ${REPO_HOME}/scripts/


