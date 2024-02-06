# very preliminary, will add external clp, folder checks during execution...
INPUT_FILE="/eos/user/e/evilla/dune/sn-data/prodmarley_nue_spectrum_clean_dune10kt_1x2x6_ES-TPdump_standardHF_noiseless_MCtruth-400events_0/tpstream_standardHF_thresh30_nonoise_MCtruth.txt"
OUTPUT_FOLDER="/eos/user/d/dapullia/tp_dataset/emaprod/test_full_event/"
SHOW=false
SAVE_IMG=true
SAVE_DS=false
WRITE=false
IMG_SAVE_FOLDER=images/
IMG_SAVE_NAME=image
N_EVENTS=50000
MIN_TPS_TO_GROUP=2
DRIFT_DIRECTION=0
MAKE_FIXED_SIZE=false
PREPROCESS_EMA_DS=true
USE_SPARSE=false

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

# if preprocess_ema_ds is true then add --preprocess_ema_ds
if [ "$PREPROCESS_EMA_DS" = true ] ; then
    PREPROCESS_EMA_DS_FLAG="--preprocess_ema_ds"
else
    PREPROCESS_EMA_DS_FLAG=""
fi

# if use_sparse is true then add --use_sparse
if [ "$USE_SPARSE" = true ] ; then
    USE_SPARSE_FLAG="--use_sparse"
else
    USE_SPARSE_FLAG=""
fi



# load a recent python version
#scl enable rh-python38 bash

# move to the folder, run and come back to scripts
cd ../python/tps_text_to_image
python create_images_from_tps.py --input_file $INPUT_FILE --output_path $OUTPUT_FOLDER --img_save_folder $IMG_SAVE_FOLDER --img_save_name $IMG_SAVE_NAME --n_events $N_EVENTS --min_tps_to_group $MIN_TPS_TO_GROUP --drift_direction $DRIFT_DIRECTION $SHOW_FLAG $SAVE_IMG_FLAG $SAVE_DS_FLAG $WRITE_FLAG $MAKE_FIXED_SIZE_FLAG $PREPROCESS_EMA_DS_FLAG $USE_SPARSE_FLAG
cd ../../scripts

