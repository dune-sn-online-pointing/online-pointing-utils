INPUT_FILE=/eos/user/d/dapullia/tpstream_hdf5/tpstream_run020638_0000_tpwriter_tpswriter_20230314T222757.hdf5
OUTPUT_PATH=/eos/user/d/dapullia/tpstream_hdf5/
FORMAT="txt npy"
NUM_RECORDS=5

python main.py --input_file $INPUT_FILE --output_path $OUTPUT_PATH --format $FORMAT --num_records $NUM_RECORDS