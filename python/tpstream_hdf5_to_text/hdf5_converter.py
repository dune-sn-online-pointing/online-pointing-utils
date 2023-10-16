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
sys.path.append('../tps_text_to_image')
import create_images_from_tps_libs as tp2img


import argparse
import sys
import numpy as np

parser = argparse.ArgumentParser(description='Convert TPStream HDF5 file to different formats.')
parser.add_argument('--input_file', type=str, help='Input file name', default='/eos/user/d/dapullia/tpstream_hdf5/tpstream_run020638_0000_tpwriter_tpswriter_20230314T222757.hdf5')
parser.add_argument('--output_path', type=str, help='Output file path', default='')
parser.add_argument('--format', type=str, help='Output file format', default=['txt'], nargs='+')
parser.add_argument('--num_records', type=int, help='Number of records to process', default=-1)
parser.add_argument('--chanmap', type=str, default='FDHDChannelMap_v1_wireends.txt', help='path to the file with Channel Map')
parser.add_argument('--min_tps_to_group', type=int, default=4, help='minimum number of TPs to create a group')
parser.add_argument('--drift_direction', type=int, default=0, help='0 for horizontal drift, 1 for vertical drift')
parser.add_argument('--ticks_limit', type=int, default=150, help='closeness in ticks to group TPs')
parser.add_argument('--channel_limit', type=int, default=20, help='closeness in channels to group TPs')
parser.add_argument('--make_fixed_size', action='store_true', help='make the image size fixed')
parser.add_argument('--img_width', type=int, default=70, help='width of the image')
parser.add_argument('--img_height', type=int, default=1000, help='height of the image')
parser.add_argument('--x_margin', type=int, default=5, help='margin in x')
parser.add_argument('--y_margin', type=int, default=50, help='margin in y')
parser.add_argument('--min_tps_to_create_img', type=int, default=2, help='minimum number of TPs to create an image')
parser.add_argument('--img_save_folder', type=str, default='images/', help='folder to save the image, on top of the output path')
parser.add_argument('--img_save_name', type=str, default='image', help='name to save the image')

args = parser.parse_args()
input_file = args.input_file
output_path = args.output_path
num_records = args.num_records
out_format = args.format
channel_map_file = args.chanmap
drift_direction = args.drift_direction
ticks_limit = args.ticks_limit
channel_limit = args.channel_limit
min_tps_to_group = args.min_tps_to_group
make_fixed_size = args.make_fixed_size
width = args.img_width
height = args.img_height
x_margin = args.x_margin
y_margin = args.y_margin
min_tps_to_create_img = args.min_tps_to_create_img
img_save_folder = args.img_save_folder
img_save_name = args.img_save_name

# check if the output format is a subset of the valid formats
valid_formats = ["txt", "npy", "img_groups", "img_all"]

save_txt = False
save_npy = False
save_img_groups = False
save_img_all = False


if not all(item in valid_formats for item in out_format):
    print(f'Output format {out_format} not valid. Please choose between txt, npy or both.')
    sys.exit(1)

if "txt" in out_format:
    print("Will save to txt file")
    save_txt = True
if "npy" in out_format:
    print("Will save to npy file")
    save_npy = True
if "img_groups" in out_format:
    print("Will save an image for each group of TPs")
    save_img_groups = True
if "img_all" in out_format:
    print("Will save one big image including all TPs")
    save_img_all = True

all_tps = tpsconv.tpstream_hdf5_converter(input_file, output_path, num_records, out_format)



# Save the data
# take the input name from the last slash

output_file_name = input_file.split("/")[-1]
print ("output file name is", output_file_name[:-5] )
if save_txt:
    np.savetxt(output_path + output_file_name[:-5] + ".txt", all_tps, fmt='%i')
if save_npy:
    np.save(output_path + output_file_name[:-5] + ".npy", all_tps)
if save_img_groups:
    channel_map = tp2img.create_channel_map_array(channel_map_file, drift_direction=drift_direction)
    groups = tp2img.group_maker(all_TPs, channel_map, ticks_limit=ticks_limit, channel_limit=channel_limit, min_tps_to_group=min_tps_to_group)
    for i, group in enumerate(groups):
        tp2img.save_img(np.array(group), channel_map, save_path=output_path+img_save_folder, outname=img_save_name+str(i), min_tps_to_create_img=min_tps_to_create_img, make_fixed_size=make_fixed_size, width=width, height=height, x_margin=x_margin, y_margin=y_margin)
if save_img_all:
    channel_map = tp2img.create_channel_map_array(channel_map_file, drift_direction=drift_direction)
    tp2img.save_img(np.array(all_tps), channel_map, save_path=output_path+img_save_folder, outname="all_" + img_save_name, min_tps_to_create_img=min_tps_to_create_img, make_fixed_size=False, width=width, height=height, x_margin=x_margin, y_margin=y_margin)



