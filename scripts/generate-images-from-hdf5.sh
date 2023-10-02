# Executing two steps at once, using only one command line input

# very preliminary, will add external clp, folder checks during execution...

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
    echo "Usage: ./generate-images-from-hdf5.sh -i <input_file> -o <output_folder> [-h]"
    exit 0
fi


./convert-hdf5-to-text.sh -i 
./create-images-from-tps.sh


