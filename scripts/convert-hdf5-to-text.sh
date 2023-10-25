# very preliminary, will add external clp, folder checks during execution...
export PYTHONPATH=$PYTHONPATH:/nfs/home/dapullia/mypltinstall/lib/python3.10/site-packages

SETTINGS_FILE="../../settings/default.json"

# Function to print help message
print_help() {
    echo "*****************************************************************************"
    echo "Usage: ./convert-hdf5-to-text.sh -s <settings_file> [-h]" 
    echo "  -s  settings file"
    echo "  -h  print this help message"
    echo "*****************************************************************************"
    exit 0
}

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -s|--settings_file)
            SETTINGS_FILE="$2"
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

# running actual command. Need to change from relative path to absolute path in repo
cd ../python/tpstream_hdf5_to_text
python hdf5_converter.py --settings_file $SETTINGS_FILE 
cd ../../scripts

