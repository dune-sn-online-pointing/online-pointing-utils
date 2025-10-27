import numpy as np
import matplotlib.pyplot as plt
import os
import sys
from pathlib import Path
sys.path.append(str(Path(__file__).parent.parent / 'lib'))
import json
import argparse
import matplotlib.pylab as pylab

sys.path.append('../python/') 
from image_creator import *
from utils import *
from cluster import *
from dataset_creator import *
from cluster_var_plotter import *

parser = argparse.ArgumentParser(description='Plot Cluster Variables')
parser.add_argument('-i', '--input-file',       type=str, help='Input root file', required=False)
parser.add_argument('-j', '--json',             type=str, help='Input json file')
parser.add_argument('-o', '--output-folder',    type=str, help='Output folder')
args = parser.parse_args()


    
input_json_file = args.input_json
output_folder = args.output_folder

if args.input_file:
    filename = args.input_file
else:
    filename = input_json['input_file']

if not os.path.exists(output_folder):   
    os.makedirs(output_folder)

# Read input json
with open(input_json_file) as f:
    input_json = json.load(f)

plane = input_json['plane']
setting = input_json['setting']

print (f"Reading input file: {filename}")
clusters, event_number = read_root_file_to_clusters(filename)


# hist_n_tps_per_cluster(clusters, output_folder, setting=setting)
# print("Done n_tps_per_cluster")
# hist_total_charge_per_cluster(clusters, output_folder, setting=setting)
# print("Done total_charge_per_cluster")
# hist_max_charge_per_cluster(clusters, output_folder, setting=setting)
# print("Done max_charge_per_cluster")
# hist_total_lenght_per_cluster(clusters, output_folder, setting=setting)
# print("Done total_lenght_per_cluster")
# hist_2D_ntps_total_charge_per_cluster(clusters, output_folder, setting=setting)
# print("Done 2D_ntps_total_charge_per_cluster")
# hist_n_sn_clusters_per_event(clusters, output_folder, setting=setting)
# print("Done n_sn_clusters_per_event")



print("Done")

