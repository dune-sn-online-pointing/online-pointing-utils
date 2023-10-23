# very preliminary, will add external clp, folder checks during execution...

INPUT_FILE="/nfs/home/dapullia/data/tpstream_run020638_0000_tpwriter_tpswriter_20230314T222757.hdf5"
OUTPUT_FOLDER="/nfs/home/dapullia/data/"
FORMAT="txt img_all" # can choose between txt and npy, leaving this for now
NUM_RECORDS=1 # leaving hardcoded for now
DRIFT_DIRECTION=1
TIME_START_FOR_IMG_ALL=104927054630266287
TIME_END_FOR_IMG_ALL=104927054630289287
export PYTHONPATH=$PYTHONPATH:/nfs/home/dapullia/mypltinstall/lib/python3.10/site-packages


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

# need to setup daq environment for this, need to have access to cvmfs
# source setup-dune-daq.sh

# running actual command. Need to change from relative path to absolute path in repo
cd ../python/tpstream_hdf5_to_text
python hdf5_converter.py --input_file $INPUT_FILE --output_path $OUTPUT_FOLDER --format $FORMAT --num_records $NUM_RECORDS --drift_direction $DRIFT_DIRECTION --time_start_for_img_all $TIME_START_FOR_IMG_ALL --time_end_for_img_all $TIME_END_FOR_IMG_ALL
cd ../../scripts
# exit the environment 
# deactivate
