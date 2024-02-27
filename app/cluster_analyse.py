"""
 * @file create_image.py
 * @brief Creates 2D images of TPs, plotting channel vs time. Colors represent the charge of the TP.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2024.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
"""

import numpy as np
import matplotlib.pyplot as plt
import time
import os
import argparse
import warnings
import gc

import sys
sys.path.append('../python/') 
# from utils import save_tps_array, create_channel_map_array
# from hdf5_converter import convert_tpstream_to_numpy 
# from image_creator import save_image, show_image
# from cluster_maker import make_clusters

import cluster


# parser for the arguments
parser = argparse.ArgumentParser(description='Tranforms Trigger Primitives to images.')
parser.add_argument('-i', '--input-file',           type=str,                         help='Input file name')
parser.add_argument('-o', '--output-path',          type=str,    default='./',        help='path to save the image')
parser.add_argument('-n', '--n-tps',                type=int,    default=1,           help='number of tps to process')
parser.add_argument('-t', '--ticks-limit',          type=int,    default=3,           help='closeness in ticks to cluster TPs')
parser.add_argument('-c', '--channel-limit',        type=int,    default=1,           help='closeness in channels to cluster TPs')
parser.add_argument('-m', '--min-tps',              type=int,    default=3,           help='minimum number of TPs to create a cluster and then an image')
parser.add_argument('--channel-map',                type=str,    default="APA",       help='"APA", "CRP" or "50L"')
parser.add_argument('--save-clusters',                action='store_true',              help='write the clusters to a textfile')
parser.add_argument('--show',                       action='store_true',              help='show the image')
parser.add_argument('--fixed-size',                 action='store_true',              help='make the image size fixed')
parser.add_argument('--img-width',                  type=int,    default=200,          help='width of the image, if fixed size')
parser.add_argument('--img-height',                 type=int,    default=300,         help='height of the image, if fixed size')
parser.add_argument('--x-margin',                   type=int,    default=2,           help='margin in x')
parser.add_argument('--y-margin',                   type=int,    default=2,          help='margin in y')
parser.add_argument('-v', '--verbose',              action='store_true',              help='verbose mode')

args = parser.parse_args()
input_file              = args.input_file
output_path             = args.output_path
show                    = args.show
n_tps                   = args.n_tps
ticks_limit             = args.ticks_limit
channel_limit           = args.channel_limit
min_tps                 = args.min_tps
channel_map             = args.channel_map
save_clusters             = args.save_clusters
fixed_size              = args.fixed_size
width                   = args.img_width
height                  = args.img_height
x_margin                = args.x_margin
y_margin                = args.y_margin
verbose                 = args.verbose

# recap of selected options
print ("#############################################")
print("Selected options:")
print(" - Input file: " + str(input_file))
print(" - Output path: " + str(output_path))
print(" - Number of TPs: " + str(n_tps))
print(" - Ticks limit: " + str(ticks_limit))
print(" - Channel limit: " + str(channel_limit))
print(" - Minimum number of TPs to create a cluster: " + str(min_tps))
print(" - Channel Map: " + str(channel_map))
print(" - Save clusters: " + str(save_clusters))
print(" - Show image: " + str(show))
print(" - Fixed size: " + str(fixed_size))
print(" - Image width: " + str(width))
print(" - Image height: " + str(height))
print(" - X margin: " + str(x_margin))
print(" - Y margin: " + str(y_margin))
print(" - Verbose: " + str(verbose))
print ("#############################################")
print (" ")

# reading with this function already orders by time_start
# all_TPs = save_tps_array(input_file, max_tps=n_tps) 

if verbose:
    print("TPs read from file: ", input_file)
    print("Number of TPs: ", len(all_TPs))
    print(" ")

# create channel map to distinguish induction and collection
# my_channel_map = create_channel_map_array(which_channel_map=channel_map)

clusters, n_events = read_root_file_to_clusters('/afs/cern.ch/work/h/hakins/private/online-pointing-utils/scripts/output/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1.root')

print("Number of clusters: ", len(clusters))
print("First cluster entry: " + str(clusters[0]))

total_charges = [] #array of total charges per cluster

for cluster in clusters:
    charge_sum = 0
    for event in cluster:
        charge_sum = charge_sum + event[4]

    total_charges.append(charge_sum)

print("total charge per cluster: " + str(total_charges))

plt.hist(total_charges, bins=5, edgecolor='black')  
plt.xlabel('Total Charge per cluster [ADC]')
plt.ylabel('Number of Clusters')
plt.title('')
plt.grid(False)

plt.savefig('plots/histogram.png')

#copy plot to local: scp hakins@lxplus.cern.ch:/afs/cern.ch/user/h/hakins/tpgtools/scripts/plots/histogram.png C:\Users\harry\Documents\Test_Plots