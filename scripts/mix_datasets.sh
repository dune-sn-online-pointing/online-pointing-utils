
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
# python dataset_mixer.py --input_json ${REPO_HOME}/json/ds_mix/ds-mix-mt-vs-all.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/ds-mix-mt-vs-all/adc_80000/

python dataset_mixer.py --input_json ${REPO_HOME}/json/ds_mix/ds-mix-es-cc-volume.json --output_folder /eos/user/d/dapullia/dune/npy_datasets/ds-mix-es-cc-volume/

cd ${REPO_HOME}/scripts/


