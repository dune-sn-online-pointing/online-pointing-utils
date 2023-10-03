'''
File name: main.py
Author: Dario Pullia
Date created: 29/09/2023

Description:
This file contains the main function that converts a HDF5 file containing TPStream data from DUNE DAQ to different formats.


Usage:
python tpstream_hdf5_converter.py -input_file <input_file_name> -input_path <input_file_path> -output_path <output_file_path> -format <output_file_format>

'''
import daqdataformats
import detdataformats
import fddetdataformats
import trgdataformats
import detchannelmaps
from hdf5libs import HDF5RawDataFile
import hdf5_converter_libs as tpsconv


import argparse
import sys
import numpy as np

parser = argparse.ArgumentParser(description='Convert TPStream HDF5 file to different formats.')
parser.add_argument('--input_file', type=str, help='Input file name', default='/eos/user/d/dapullia/tpstream_hdf5/tpstream_run020638_0000_tpwriter_tpswriter_20230314T222757.hdf5')
parser.add_argument('--output_path', type=str, help='Output file path', default='/eos/user/d/dapullia/tpstream_hdf5/')
parser.add_argument('--format', type=str, help='Output file format', default=['txt'], nargs='+')
parser.add_argument('--num_records', type=int, help='Number of records to process', default=-1)

args = parser.parse_args()
input_file = args.input_file
output_path = args.output_path
num_records = args.num_records
out_format = args.format

# check if the output format is a subset of the valid formats
valid_formats = ["txt", "npy"]

save_txt = False
save_npy = False

if not all(item in valid_formats for item in out_format):
    print(f'Output format {out_format} not valid. Please choose between txt, npy or both.')
    sys.exit(1)

if "txt" in out_format:
    print("Will save to txt file")
    save_txt = True
if "npy" in out_format:
    print("Will save to npy file")
    save_npy = True


all_tps = tpsconv.tpstream_hdf5_converter(input_file, output_path, num_records, out_format)



# Save the data
# take the input name from the last slash

output_file_name = input_file.split("/")[-1]
print ("output file name is", output_file_name[:-5] )
if save_txt:
    np.savetxt(output_path + output_file_name[:-5] + ".txt", all_tps, fmt='%i')
if save_npy:
    np.save(output_path + output_file_name[:-5] + ".npy", all_tps)

