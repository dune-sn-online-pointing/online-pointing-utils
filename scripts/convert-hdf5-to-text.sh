INPUT_FILE=""
OUTPUT_PATH=""
FORMAT="npy" # can chosse between txt and numpy, leaving this for now
NUM_RECORDS=5 # leaving hardcoded for now


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
        -i|--input-file)
            INPUT_FILE="$2"
            shift 2
            ;;
        -o|--output-path)
            work_dir="$2"
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

# if work_dir="" or dunedaq_release="" stop execution
if [ -z "$work_dir" ] || [ -z "$dunedaq_release" ]
then
    echo "Usage: create-dunedaq.sh -d <work_dir> -v <dunedaq_release>"
    exit 0
fi

# running first command
python main.py --input_file $INPUT_FILE --output_path $OUTPUT_PATH --format $FORMAT --num_records $NUM_RECORDS
