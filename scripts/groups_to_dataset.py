import numpy as np
import matplotlib.pyplot as plt
import sys
import argparse

sys.path.append('../python/') 
from image_creator import *
from utils import *
from group import *
from dataset_creator import *


parser = argparse.ArgumentParser(description='Tranforms Trigger Primitives to images.')
parser.add_argument('--input_file', type=str, help='Input file name')
parser.add_argument('--output_path', type=str, default='/eos/user/d/dapullia/tp_dataset/', help='path to save the image')
parser.add_argument('--make_fixed_size', action='store_true', help='make the image size fixed')
parser.add_argument('--img_width', type=int, default=70, help='width of the image')
parser.add_argument('--img_height', type=int, default=300, help='height of the image')
parser.add_argument('--x_margin', type=int, default=5, help='margin in x')
parser.add_argument('--y_margin', type=int, default=10, help='margin in y')
parser.add_argument('--drift_direction', type=int, default=0, help='0 for horizontal drift, 1 for vertical drift')
parser.add_argument('--min_tps_to_group', type=int, default=2, help='minimum number of TPs to create a group')
parser.add_argument('--save_img_dataset', type=int, default=0, help='save the image dataset')
parser.add_argument('--save_process_label', type=int, default=0, help='label is process')
parser.add_argument('--save_true_dir_label', type=int, default=0, help='label is true direction')

args = parser.parse_args()
filename = args.input_file
output_path = args.output_path
make_fixed_size = args.make_fixed_size
width = args.img_width
height = args.img_height
x_margin = args.x_margin
y_margin = args.y_margin
drift_direction = args.drift_direction
min_tps_to_group = args.min_tps_to_group
save_img_dataset = args.save_img_dataset
save_process_label = args.save_process_label
save_true_dir_label = args.save_true_dir_label
dt = np.dtype([('time_start', float), 
                ('time_over_threshold', float),
                ('time_peak', float),
                ('channel', int),
                ('adc_integral', int),
                ('adc_peak', int),
                ('detid', int),
                ('type', int),
                ('algorithm', int),
                ('version', int),
                ('flag', int)])


if __name__ == '__main__':
    # Read the root file
    groups, event_number = read_root_file_to_groups(filename)

    print(f"Number of groups: {len(groups)}")
    # Create the channel map
    channel_map = create_channel_map_array(which_detector="APA")
    # Prepare the output path   
    if not os.path.exists(output_path + 'dataset'):
        os.makedirs(output_path + 'dataset')

    # Create the images
    if save_img_dataset:
        print("Creating the images")
        dataset_img = create_dataset_img(groups=groups, channel_map=channel_map, min_tps_to_create_img=min_tps_to_group, make_fixed_size=True, width=width, height=height, x_margin=x_margin, y_margin=y_margin, only_collection=True)
        print(f"Shape of the dataset_img: {dataset_img.shape}")
        np.save(output_path + 'dataset/dataset_img.npy', dataset_img)
        save_samples_from_ds(dataset_img, output_path + 'samples/', n_samples=10)
    # Create the labels
    if save_process_label:
        print("Creating the process labels")
        dataset_label_process = create_dataset_label_process(groups)
        print(f"Shape of the dataset_label_process: {dataset_label_process.shape}")
        print(f"Unique labels: {np.unique(dataset_label_process, return_counts=True)}")
        np.save(output_path + 'dataset/dataset_label_process.npy', dataset_label_process)
    if save_true_dir_label:
        print("Creating the true direction labels")
        dataset_label_true_dir = create_dataset_label_true_dir(groups)
        print(f"Shape of the dataset_label_true_dir: {dataset_label_true_dir.shape}")
        np.save(output_path + 'dataset/dataset_label_true_dir.npy', dataset_label_true_dir)


    print('Done!')