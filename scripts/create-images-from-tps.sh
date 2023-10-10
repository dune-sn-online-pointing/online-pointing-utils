# very preliminary, will add external clp, folder checks during execution...

INPUT_FILE="/eos/user/d/dapullia/tp_dataset/snana_hits.txt"
OUTPUT_FOLDER="/eos/user/d/dapullia/tp_dataset/snana/temp_test/"
CHANMAP=../../channel-maps/channel_map_upright.txt # leaving hardcoded for now
SHOW=false
SAVE_IMG=true
SAVE_DS=true
WRITE=true 
IMG_SAVE_FOLDER=images/
IMG_SAVE_NAME=image
N_EVENTS=10000
MIN_TPS_TO_GROUP=2
DRIFT_DIRECTION=0
MAKE_FIXED_SIZE=true


# Function to print help message
print_help() {
    echo "*****************************************************************************"
    echo "Usage: generate-images-from-hdf5.sh -i <input_file> -o <output_folder> [-h]"
    echo "  -i  input file"
    echo "  -o  output folder"
    echo "  -h  print this help message"
    echo "*****************************************************************************"
    exit 0
}

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -i|--input_file)
            INPUT_FILE="$2"
            shift 2
            ;;
        -o|--OUTPUT_FOLDER)
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

# stop execution if fundamental variables are not set
if [ -z "$INPUT_FILE" ] || [ -z "$OUTPUT_FOLDER" ]
then
    echo "Usage: ./create-images-from-tps.sh -i <input_file> -o <output_folder> [-h]"
    exit 0
fi



# if show is true then add --show
if [ "$SHOW" = true ] ; then
    SHOW_FLAG="--show"
else
    SHOW_FLAG=""
fi

# if save_img is true then add --save_img
if [ "$SAVE_IMG" = true ] ; then
    SAVE_IMG_FLAG="--save_img"
else
    SAVE_IMG_FLAG=""
fi

# if save_ds is true then add --save_ds
if [ "$SAVE_DS" = true ] ; then
    SAVE_DS_FLAG="--save_ds"
else
    SAVE_DS_FLAG=""
fi

# if write is true then add --write
if [ "$WRITE" = true ] ; then
    WRITE_FLAG="--write"
else
    WRITE_FLAG=""
fi

# if make_fixed_size is true then add --make_fixed_size
if [ "$MAKE_FIXED_SIZE" = true ] ; then
    MAKE_FIXED_SIZE_FLAG="--make_fixed_size"
else
    MAKE_FIXED_SIZE_FLAG=""
fi


# load a recent python version
#scl enable rh-python38 bash

# move to the folder, run and come back to scripts
cd ../python/tps_text_to_image
python create_images_from_tps.py --input_file $INPUT_FILE --chanmap $CHANMAP --output_path $OUTPUT_FOLDER --img_save_folder $IMG_SAVE_FOLDER --img_save_name $IMG_SAVE_NAME --n_events $N_EVENTS --min_tps_to_group $MIN_TPS_TO_GROUP --drift_direction $DRIFT_DIRECTION $SHOW_FLAG $SAVE_IMG_FLAG $SAVE_DS_FLAG $WRITE_FLAG $MAKE_FIXED_SIZE_FLAG
cd ../../scripts

