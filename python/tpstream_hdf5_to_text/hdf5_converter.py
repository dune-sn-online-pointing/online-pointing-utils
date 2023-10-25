'''
File name: main.py
Author: Dario Pullia
Date created: 29/09/2023

Description:
This file contains the main function that converts a HDF5 file containing TPStream data from DUNE DAQ to different formats.

'''
import daqdataformats
import detdataformats
import fddetdataformats
import trgdataformats
import detchannelmaps
from hdf5libs import HDF5RawDataFile
import hdf5_converter_libs as tpsconv
import sys


import json
import argparse
import sys
import numpy as np

parser = argparse.ArgumentParser(description='Convert TPStream HDF5 file to different formats.')
parser.add_argument('--settings_file', type=str, help='Settings file path', default='your_config.json')

args = parser.parse_args()
settings_file = args.settings_file

# Specify the path to your JSON configuration file
json_settings = settings_file
# Open and read the JSON file
with open(json_settings, "r") as json_file:
    config_data = json.load(json_file)

# Access individual values from the config
input_file = config_data["INPUT_FILE"]
output_folder = config_data["OUTPUT_FOLDER"]

if input_file == '' or output_folder == '':
    print("Please specify input file and output folder")
    sys.exit(1)


out_format = config_data["FORMAT"]
num_records = config_data["NUM_RECORDS"]
channel_map_file = config_data["CHANNEL_MAP"]
min_tps_to_group = config_data["MIN_TPS_TO_GROUP"]
drift_direction = config_data["DRIFT_DIRECTION"]
ticks_limit = config_data["TICKS_LIMIT"]
channel_limit = config_data["CHANNEL_LIMIT"]
make_fixed_size = config_data["MAKE_FIXED_SIZE"]
img_width = config_data["IMG_WIDTH"]
img_height = config_data["IMG_HEIGHT"]
x_margin = config_data["X_MARGIN"]
y_margin = config_data["Y_MARGIN"]
min_tps_to_create_img = config_data["MIN_TPS_TO_CREATE_IMG"]
img_save_folder = config_data["IMG_SAVE_FOLDER"]
img_save_name = config_data["IMG_SAVE_NAME"]
time_start = config_data["TIME_START"]
time_end = config_data["TIME_END"]

# check if the output format is a subset of the valid formats
out_format = out_format.split(" ")
valid_formats = ["txt", "npy", "img_groups", "img_all"]

save_txt = False
save_npy = False
save_img_groups = False
save_img_all = False


if not all(item in valid_formats for item in out_format):
    print(f'Output format {out_format} not valid. Please choose between txt, npy, img_groups, img_all')
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

if save_img_groups or save_img_all:
    sys.path.append('../tps_text_to_image')
    import create_images_from_tps_libs as tp2img


all_tps = tpsconv.tpstream_hdf5_converter(input_file, num_records, out_format)

print(f"Converted {all_tps.shape[0]} tps!")
# Save the data

eff_time_start=all_tps[0][0]
eff_time_end=all_tps[-1][0]
all_tps = np.array(all_tps)
if not (eff_time_start < time_start and time_start<time_end and time_end<eff_time_end):
    print("Something wrong with your time choices. You have:")
    print(f"First TP time start: {eff_time_start}")
    print(f"First TP time end: {eff_time_end}")
    print(f"I will draw all the converted TPs.")
else:
    print(f"I will draw all TPs from {time_start} to {time_end}.")
    all_tps = all_tps[(all_tps[:, 0] >= time_start) & (all_tps[:, 0] <= time_end)]

# take the input name from the last slash
output_file_name = input_file.split("/")[-1]
print ("output file name is", output_file_name[:-5] )
if save_txt:
    np.savetxt(output_folder + output_file_name[:-5] + ".txt", all_tps, fmt='%i')
if save_npy:
    np.save(output_folder + output_file_name[:-5] + ".npy", all_tps)
if save_img_groups:
    print("Producing groups images")
    channel_map = tp2img.create_channel_map_array(channel_map_file, drift_direction=drift_direction)
    groups = tp2img.group_maker(all_tps, channel_map, ticks_limit=ticks_limit, channel_limit=channel_limit, min_tps_to_group=min_tps_to_group)
    print(f"Created {len(groups)} groups!")

    for i, group in enumerate(groups):
        tp2img.save_img(np.array(group), channel_map, save_path=output_folder+img_save_folder, outname=img_save_name+str(i), min_tps_to_create_img=min_tps_to_create_img, make_fixed_size=make_fixed_size, width=img_width, height=img_height, x_margin=x_margin, y_margin=y_margin)
if save_img_all:
    print("Producing all tps image")
    channel_map = tp2img.create_channel_map_array(channel_map_file, drift_direction=drift_direction)
    tp2img.save_img((all_tps), channel_map, save_path=output_folder+img_save_folder, outname="all_" + img_save_name, min_tps_to_create_img=min_tps_to_create_img, make_fixed_size=True, width=2*img_width, height=2*img_height, x_margin=x_margin, y_margin=y_margin)



