# very preliminary, will add external clp, folder checks during execution...

INPUT_FILE=""
OUTPUT_FOLDER=""
FORMAT="npy" # can choose between txt and npy, leaving this for now
NUM_RECORDS=5 # leaving hardcoded for now


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
source setup-dune-daq.sh

# running actual command. Need to change from relative path to absolute path in repo
python ../python/hdf5_converter.py --input_file $INPUT_FILE --output_folder $OUTPUT_FOLDER --format $FORMAT --num_records $NUM_RECORDS

# exit the environment 
deactivate
