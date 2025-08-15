import numpy as np
import matplotlib.pyplot as plt
import os
import sys
import json
import argparse
import matplotlib.pylab as pylab

sys.path.append('../python/') 
from image_creator import *
from utils import *
from cluster import *
from dataset_creator import *

# Set seed for reproducibility
np.random.seed(0)


parser = argparse.ArgumentParser(description='Plot Cluster Position')
parser.add_argument('--input_json', type=str, help='Input json file')
parser.add_argument('--output_folder', type=str, help='Output folder')
args = parser.parse_args()

input_json_file = args.input_json
output_folder = args.output_folder

if not os.path.exists(output_folder):   
    os.makedirs(output_folder)

with open(input_json_file) as f:
    input_json = json.load(f)

filename = input_json['input_file']
radius = input_json['radius']

clusters, event_number = read_root_file_to_clusters(filename)

unique_event_numbers = np.unique(event_number)

tot_n_clusters = []
tot_n_sn_clusters = []
tot_n_sn_clusters_in_radius = []
tot_n_clusters_in_radius = []

distance_from_main_track = []


clusters_ev = {}
main_clusters = {}

for cluster in clusters:
    if cluster.get_true_event_number() in clusters_ev:
        clusters_ev[cluster.get_true_event_number()].append(cluster)
    else:
        clusters_ev[cluster.get_true_event_number()] = [cluster]
    if cluster.get_true_label() == 100 or cluster.get_true_label() == 101:
        main_clusters[cluster.get_true_event_number()] = cluster

img_counter_cc = 0
img_counter_es = 0

fontsize = 17


# take only the common keys
unique_event_numbers = np.intersect1d(list(clusters_ev.keys()), list(main_clusters.keys()))


for event_number in unique_event_numbers:
    clusters_event = clusters_ev[event_number]
    main_cluster = main_clusters[event_number]
    n_clusters = len(clusters_event)
    sn_clusters = [cluster for cluster in clusters_event if cluster.get_supernova_tp_fraction() > 0]
    n_sn_clusters = len(sn_clusters)
    clusters_in_radius = [cluster for cluster in clusters_event if distance_between_clusters(cluster, main_cluster) < radius]
    n_clusters_in_radius = len(clusters_in_radius)
    sn_clusters_in_radius = [cluster for cluster in clusters_in_radius if cluster.get_supernova_tp_fraction() > 0]
    n_sn_clusters_in_radius = len(sn_clusters_in_radius)

    tot_n_clusters.append(n_clusters)
    tot_n_sn_clusters.append(n_sn_clusters)
    tot_n_clusters_in_radius.append(n_clusters_in_radius)
    tot_n_sn_clusters_in_radius.append(n_sn_clusters_in_radius)
    distance_from_main_track_ev = [distance_between_clusters(cluster, main_cluster) for cluster in clusters_event if cluster.get_supernova_tp_fraction() > 0]
    distance_from_main_track = distance_from_main_track + distance_from_main_track_ev

    if main_cluster.get_true_label() == 100:
        if img_counter_cc < 5:
            plt.figure(figsize=(12,12))
            plt.scatter([cluster.get_reco_pos()[0] for cluster in clusters_event], [cluster.get_reco_pos()[2] for cluster in clusters_event], s=2, label=f"N. clusters: {n_clusters}")
            plt.scatter([cluster.get_reco_pos()[0] for cluster in clusters_in_radius], [cluster.get_reco_pos()[2] for cluster in clusters_in_radius], s=2, label=f"N. clusters within {radius} cm: {n_clusters_in_radius}")
            plt.scatter([cluster.get_reco_pos()[0] for cluster in sn_clusters], [cluster.get_reco_pos()[2] for cluster in sn_clusters], s=10, label=f"N. supernova clusters: {n_sn_clusters}")
            plt.scatter([cluster.get_reco_pos()[0] for cluster in sn_clusters_in_radius], [cluster.get_reco_pos()[2] for cluster in sn_clusters_in_radius], s=10, label=f"N. supernova clusters within {radius} cm: {n_sn_clusters_in_radius}")
            plt.scatter(main_cluster.get_reco_pos()[0], main_cluster.get_reco_pos()[2], s=30, label="Main cluster")


            plt.legend(fontsize=fontsize)
            plt.xlabel("X [cm]", fontsize=fontsize)
            plt.ylabel("Z [cm]", fontsize=fontsize)
            # set ticks font size
            plt.xticks(fontsize=fontsize)
            plt.yticks(fontsize=fontsize)
            plt.savefig(output_folder + f"cluster_positions_cc_{img_counter_cc}.png")
            plt.clf()
            plt.close()
            
            img_counter_cc += 1
    elif main_cluster.get_true_label() == 101:
        if img_counter_es < 5:
            plt.figure(figsize=(12,12))
            plt.scatter([cluster.get_reco_pos()[0] for cluster in clusters_event], [cluster.get_reco_pos()[2] for cluster in clusters_event], s=2, label=f"N. clusters: {n_clusters}")
            plt.scatter([cluster.get_reco_pos()[0] for cluster in clusters_in_radius], [cluster.get_reco_pos()[2] for cluster in clusters_in_radius], s=2, label=f"N. clusters within {radius} cm: {n_clusters_in_radius}")
            plt.scatter([cluster.get_reco_pos()[0] for cluster in sn_clusters], [cluster.get_reco_pos()[2] for cluster in sn_clusters], s=10, label=f"N. supernova clusters: {n_sn_clusters}")
            plt.scatter([cluster.get_reco_pos()[0] for cluster in sn_clusters_in_radius], [cluster.get_reco_pos()[2] for cluster in sn_clusters_in_radius], s=10, label=f"N. supernova clusters within {radius} cm: {n_sn_clusters_in_radius}")
            plt.scatter(main_cluster.get_reco_pos()[0], main_cluster.get_reco_pos()[2], s=30, label="Main cluster")


            plt.legend(fontsize=fontsize)
            plt.xlabel("X [cm]", fontsize=fontsize)
            plt.ylabel("Z [cm]", fontsize=fontsize)
            # set ticks font size
            plt.xticks(fontsize=fontsize)
            plt.yticks(fontsize=fontsize)
            
            plt.savefig(output_folder + f"cluster_positions_es_{img_counter_es}.png")
            plt.clf()
            plt.close()

            img_counter_es += 1

n_bins = 100

plt.figure(figsize=(7,7))
plt.hist(tot_n_clusters, bins=n_bins, label="N. clusters", alpha=0.5)
plt.xlabel("N. clusters")
plt.ylabel("N. events")
plt.title("N. clusters per event")
plt.savefig(output_folder + "n_clusters.png")
plt.clf()

plt.hist(tot_n_sn_clusters, label="N. supernova clusters", alpha=0.5)
plt.xlabel("N. clusters")
plt.ylabel("N. events")
plt.title("N. supernova clusters per event")
plt.savefig(output_folder + "n_sn_clusters.png")
plt.clf()

n_bins = 80
max_n = 80
plt.hist(tot_n_clusters_in_radius, bins=n_bins, range=(0, max_n), label=f"N. clusters within {radius} cm", alpha=0.5)
plt.hist(tot_n_sn_clusters_in_radius, bins=n_bins, range=(0, max_n), label=f"N. supernova clusters within {radius} cm", alpha=0.5)
plt.legend()
plt.xlabel("N. clusters")
plt.ylabel("N. events")
plt.title(f"N. clusters within {radius} cm")
plt.savefig(output_folder + "n_clusters_in_radius.png")
plt.clf()

n_bins = 100
plt.hist(distance_from_main_track, bins=n_bins, range=(0,300), label="Distance from main track", alpha=0.5)
plt.xlabel("Distance from main track [cm]")
plt.ylabel("N. clusters")
plt.title("Distance from main track")
plt.savefig(output_folder + "distance_from_main_track.png")
plt.clf()
    


