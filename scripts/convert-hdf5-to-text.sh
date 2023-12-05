# very preliminary, will add external clp, folder checks during execution...

INPUT_FILE="/eos/user/d/dapullia/tpstream_hdf5/tpstream_run020638_0000_tpwriter_tpswriter_20230314T222757.hdf5"
OUTPUT_FOLDER="/eos/user/d/dapullia/tpstream_hdf5/utils_test_output/"
FORMAT="txt npy" # can choose between [txt img_all npy img_groups], leaving this for now
NUM_RECORDS=1 # leaving hardcoded for now
MIN_TPS_TO_GROUP=9 # leaving hardcoded for now
DRIFT_DIRECTION=1 # 0 for horizontal drift, 1 for vertical drift
TICKS_LIMIT=100 # closeness in ticks to group TPs
CHANNEL_LIMIT=20 # closeness in channels to group TPs
MAKE_FIXED_SIZE=true # make the image size fixed
IMG_WIDTH=70 # width of the image
IMG_HEIGHT=1000 # height of the image
X_MARGIN=5 # margin in x
Y_MARGIN=50 # margin in y
MIN_TPS_TO_CREATE_IMG=2 # minimum number of TPs to create an image
IMG_SAVE_FOLDER="images/" # folder to save the image, on top of the output path
IMG_SAVE_NAME="image" # name to save the image
# TIME_START=104927054630266287 # Start time to draw for IMG ALL
# TIME_END=104927054630496287 # End time to draw for IMG ALL
# N_TPS=-1 # Number of TPs to draw for IMG ALL
USE_REAL_TIMESTAMPS=false # Use real timestamps for IMG ALL
# export PYTHONPATH=$PYTHONPATH:/nfs/home/dapullia/mypltinstall/lib/python3.10/site-packages


# Function to print help message
print_help() {
    echo "*****************************************************************************"
    echo "Usage: ./convert-hdf5-to-text.sh -i <input_file> -o <output_folder> [-h]"
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
    echo "Usage: ./convert-hdf5-to-text.sh -i <input_file> -o <output_folder> [-h]"
    exit 0
fi
# if show is true then add --show
if [ "$MAKE_FIXED_SIZE" = true ] ; then
    MAKE_FIXED_SIZE="--make-fixed-size"
else
    MAKE_FIXED_SIZE=""
fi

if [ "$USE_REAL_TIMESTAMPS" = true ] ; then
    USE_REAL_TIMESTAMPS="--use-real-timestamps"
else
    USE_REAL_TIMESTAMPS=""
fi

# need to setup daq environment for this, need to have access to cvmfs
# source setup-dune-daq.sh

# running actual command. Need to change from relative path to absolute path in repo
cd ../python/tpstream_hdf5_to_text
python hdf5_converter.py --input-file $INPUT_FILE --output-folder $OUTPUT_FOLDER --format $FORMAT --num-records $NUM_RECORDS --min-tps-to-group $MIN_TPS_TO_GROUP --drift-direction $DRIFT_DIRECTION --ticks-limit $TICKS_LIMIT --channel-limit $CHANNEL_LIMIT --img-width $IMG_WIDTH --img-height $IMG_HEIGHT --x-margin $X_MARGIN --y-margin $Y_MARGIN --min-tps-to-create-img $MIN_TPS_TO_CREATE_IMG --img-save-folder $IMG_SAVE_FOLDER --img-save-name $IMG_SAVE_NAME --time-start $TIME_START --time-end $TIME_END --n-tps $N_TPS --use-real-timestamps $USE_REAL_TIMESTAMPS $MAKE_FIXED_SIZE
cd ../../scripts
# exit the environment 
# deactivate