import numpy as np
import matplotlib.pyplot as plt
import sys
import json
import argparse

sys.path.append('../python/') 
from image_creator import *
from utils import *
from cluster import *
from dataset_creator import *


parser = argparse.ArgumentParser(description='Tranforms Trigger Primitives to images.')
parser.add_argument('--input_json', type=str, help='Input json file')
parser.add_argument('--output_folder', type=str, help='Output folder')
args = parser.parse_args()

input_json_file = args.input_json
output_path = args.output_folder

# Read input json
with open(input_json_file) as f:
    input_json = json.load(f)

filename = input_json['input_file']
make_fixed_size = input_json['make_fixed_size']
width = input_json['img_width']
height = input_json['img_height']
min_tps_to_cluster = input_json['min_tps_to_cluster']
drift_direction = input_json['drift_direction']
save_img_dataset = input_json['save_img_dataset']
save_process_label = input_json['save_label_process']
save_true_dir_label = input_json['save_label_direction']
x_margin = input_json['x_margin']
y_margin = input_json['y_margin']
only_collection = input_json['only_collection']


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
    clusters, event_number = read_root_file_to_clusters(filename)

    print(f"Number of clusters: {len(clusters)}")
    # Create the channel map
    channel_map = create_channel_map_array(which_detector="APA")
    # Prepare the output path   
    if not os.path.exists(output_path + 'dataset'):
        os.makedirs(output_path + 'dataset')

    # Create the images
    if save_img_dataset:
        print("Creating the images")
        dataset_img = create_dataset_img(clusters=clusters, channel_map=channel_map, min_tps_to_create_img=min_tps_to_cluster, make_fixed_size=True, width=width, height=height, x_margin=x_margin, y_margin=y_margin, only_collection=only_collection)
        print(f"Shape of the dataset_img: {dataset_img.shape}")
        np.save(output_path + 'dataset/dataset_img.npy', dataset_img)
        save_samples_from_ds(dataset_img, output_path + 'samples/', n_samples=10)
    # Create the labels
    if save_process_label:
        print("Creating the process labels")
        dataset_label_process = create_dataset_label_process(clusters)
        print(f"Shape of the dataset_label_process: {dataset_label_process.shape}")
        print(f"Unique labels: {np.unique(dataset_label_process, return_counts=True)}")
        np.save(output_path + 'dataset/dataset_label_process.npy', dataset_label_process)
    if save_true_dir_label:
        print("Creating the true direction labels")
        dataset_label_true_dir = create_dataset_label_true_dir(clusters)
        print(f"Shape of the dataset_label_true_dir: {dataset_label_true_dir.shape}")
        np.save(output_path + 'dataset/dataset_label_true_dir.npy', dataset_label_true_dir)


    print('Done!')