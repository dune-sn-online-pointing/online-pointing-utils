'''
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

import argparse
import sys
import numpy as np

parser = argparse.ArgumentParser(description='Convert TPStream HDF5 file to different formats.')
parser.add_argument('--input-file', type=str, help='Input file name', default='')
parser.add_argument('--output-folder', type=str, help='Output file path', default='')
parser.add_argument('--input-format', type=str, help='Input file format', default='hdf5')
parser.add_argument('--format', type=str, help='Output file format', default=['txt', 'img_all'], nargs='+')
parser.add_argument('--num-records', type=int, help='Number of records to process', default=-1)
parser.add_argument('--min-tps-to-group', type=int, default=9, help='minimum number of TPs to create a group')
parser.add_argument('--drift-direction', type=int, default=1, help='0 for horizontal drift, 1 for vertical drift, 2 for 50L')
parser.add_argument('--ticks-limit', type=int, default=100, help='closeness in ticks to group TPs')
parser.add_argument('--channel-limit', type=int, default=20, help='closeness in channels to group TPs')
parser.add_argument('--make-fixed-size', action='store_true', help='make the image size fixed')
parser.add_argument('--img-width', type=int, default=250, help='width of the image')
parser.add_argument('--img-height', type=int, default=3000, help='height of the image')
parser.add_argument('--x-margin', type=int, default=30, help='margin in x')
parser.add_argument('--y-margin', type=int, default=50, help='margin in y')
parser.add_argument('--min-tps-to-create-img', type=int, default=2, help='minimum number of TPs to create an image')
parser.add_argument('--img-save-folder', type=str, default='images/', help='folder to save the image, on top of the output path')
parser.add_argument('--img-save-name', type=str, default='image', help='name to save the image')
parser.add_argument('--time-start', type=int, default=-1, help='Start time to draw for IMG ALL')
parser.add_argument('--time-end', type=int, default=-1, help='End time to draw for IMG ALL')
parser.add_argument('--n-tps',type=int, help='Number of TPs to save', default=-1)
parser.add_argument('--use-real-timestamps', action='store_true', help='Timestamps not starting from zero')

args = parser.parse_args()
input_file = args.input_file
output_folder = args.output_folder
in_format = args.input_format
out_format = args.format
num_records = args.num_records
min_tps_to_group = args.min_tps_to_group
drift_direction = args.drift_direction
ticks_limit = args.ticks_limit
channel_limit = args.channel_limit
make_fixed_size = args.make_fixed_size
img_width = args.img_width
img_height = args.img_height
x_margin = args.x_margin
y_margin = args.y_margin
min_tps_to_create_img = args.min_tps_to_create_img
img_save_folder = args.img_save_folder
img_save_name = args.img_save_name
time_start=args.time_start
time_end=args.time_end
n_tps_to_convert = args.n_tps
use_real_timestamps = args.use_real_timestamps


# check if the output format is a subset of the valid formats
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


if in_format=='hdf5':
    all_tps = tpsconv.tpstream_hdf5_converter(input_file, num_records=num_records, n_tps_to_convert=n_tps_to_convert)
    output_file_name = input_file.split("/")[-1]
    output_file_name=output_file_name[:-5]
elif in_format=='txt':
    all_tps = np.loadtxt(input_file, dtype=int, max_rows= n_tps_to_convert)
    output_file_name = input_file.split("/")[-1]
    output_file_name=output_file_name[:-4]
else:
    print("File format not valid. Use txt or hdf5.")
    exit()

if not use_real_timestamps:
    all_tps["time_start"]=all_tps["time_start"] - all_tps[0]["time_start"]
    all_tps["time_peak"]=all_tps["time_peak"] - all_tps[0]["time_start"]


print(f"Converted {all_tps.shape[0]} tps!")


eff_time_start=all_tps[0]["time_start"]
eff_time_end=all_tps[-1]["time_start"]

print(f"First TP time start: {eff_time_start}")
print(f"First TP time end: {eff_time_end}")
all_tps = np.array(all_tps)
if not (eff_time_start < time_start and time_start<time_end and time_end<eff_time_end):
    print("Something wrong with your time choices. You have:")
    print(f"First TP time start: {eff_time_start}")
    print(f"First TP time end: {eff_time_end}")
    print(f"I will draw all the converted TPs.")
else:
    print(f"I will draw all TPs from {time_start} to {time_end}.")
    all_tps = all_tps[(all_tps["time_start"] >= time_start) & (all_tps["time_start"] <= time_end)]

# take the input name from the last slash
print ("output file name is", output_file_name )
if save_txt:
    np.savetxt(output_folder + output_file_name + ".txt", all_tps, fmt='%i')
if save_npy:
    np.save(output_folder + output_file_name+ ".npy", all_tps)
if save_img_groups:
    print("Producing groups images")
    channel_map = tp2img.create_channel_map_array(drift_direction=drift_direction)
    groups = tp2img.group_maker(all_tps, channel_map, ticks_limit=ticks_limit, channel_limit=channel_limit, min_tps_to_group=min_tps_to_group)
    print(f"Created {len(groups)} groups!")

    for i, group in enumerate(groups):
        tp2img.save_img(np.array(group), channel_map, save_path=output_folder+img_save_folder, outname=img_save_name+str(i), min_tps_to_create_img=min_tps_to_create_img, make_fixed_size=make_fixed_size, width=img_width, height=img_height, x_margin=x_margin, y_margin=y_margin)
if save_img_all:
    print("Producing all tps image")
    channel_map = tp2img.create_channel_map_array(drift_direction=drift_direction)
    tp2img.save_img((all_tps), channel_map, save_path=output_folder+img_save_folder, outname="all_" + img_save_name, min_tps_to_create_img=min_tps_to_create_img, make_fixed_size=make_fixed_size, width=2*img_width, height=2*img_height, x_margin=x_margin, y_margin=y_margin)



