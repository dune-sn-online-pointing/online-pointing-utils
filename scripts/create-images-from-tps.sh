# very preliminary, will add external clp, folder checks during execution...

INPUT_FILE=""
OUTPUT_FOLDER=""
CHANMAP=vdcbce_chanmap_v4.txt # leaving hardcoded for now
SHOW=false
SAVE_IMG=false
SAVE_DS=true
WRITE=true
IMG_SAVE_FOLDER=images/
IMG_SAVE_NAME=image
N_EVENTS=1000000
MIN_TPS_TO_CLUSTER=6

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

python create-images-from-tps.py --input_file $INPUT_FILE --chanmap $CHANMAP --output_folder $OUTPUT_FOLDER --img_save_folder $IMG_SAVE_FOLDER --img_save_name $IMG_SAVE_NAME --n_events $N_EVENTS --min_tps_to_cluster $MIN_TPS_TO_CLUSTER $SHOW_FLAG $SAVE_IMG_FLAG $SAVE_DS_FLAG $WRITE_FLAG