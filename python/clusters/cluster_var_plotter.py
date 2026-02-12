import numpy as np
import matplotlib.pyplot as plt
import os
import sys
from pathlib import Path
sys.path.append(str(Path(__file__).parent.parent / 'lib'))

from image_creator import *
from utils import *
from cluster import *
from dataset_creator import *

import matplotlib.pylab as pylab
params = {'legend.fontsize': 'x-large',
          'figure.figsize': (15, 5),
         'axes.labelsize': 'x-large',
         'axes.titlesize':'x-large',
         'xtick.labelsize':'x-large',
         'ytick.labelsize':'x-large'}
pylab.rcParams.update(params)



def hist_n_tps_per_cluster(clusters, output_folder, main_track_label=77, setting="tl_3_cl_1"):
    n_tps = []
    n_tps_supernova = []
    n_tps_mostly_supernova = []
    n_tps_all_supernova = []
    n_tps_main_track = []
    for cluster in clusters:
        n_tps.append(len(cluster))
        if cluster.get_supernova_tp_fraction() > 0.:
            n_tps_supernova.append(len(cluster))
        if cluster.get_supernova_tp_fraction() > 0.5:
            n_tps_mostly_supernova.append(len(cluster))
        if cluster.get_supernova_tp_fraction() == 1:
            n_tps_all_supernova.append(len(cluster))
        if cluster.get_true_label() == main_track_label:
            n_tps_main_track.append(len(cluster))
    # find max value of the histogram
    max_value = max(n_tps)
    max_value_with_margin = int(max_value * 1.1)

    plt.figure(figsize=(11, 7))
    plt.hist(n_tps, bins=max_value_with_margin+1, range=(0, max_value_with_margin+1), label="All clusters", align='left')
    plt.hist(n_tps_supernova, bins=max_value_with_margin+1, range=(0, max_value_with_margin+1), label="Contains supernova clusters", color='red', align='left')
    plt.hist(n_tps_mostly_supernova, bins=max_value_with_margin+1, range=(0, max_value_with_margin+1), label="Mostly supernova clusters", color='orange', align='left')
    plt.hist(n_tps_all_supernova, bins=max_value_with_margin+1, range=(0, max_value_with_margin+1), label="Clean supernova clusters", color='green', align='left')
    # add patch for the longest cluster per event
    plt.hist(n_tps_main_track, bins=max_value_with_margin+1, range=(0, max_value_with_margin+1), label="Main track per event", color='purple', hatch='xx', alpha=0.1, align='left')

    plt.xlabel("Number of TPs per cluster", fontsize=18)
    plt.ylabel("Number of clusters", fontsize=18)
    plt.title(f"Number of TPs per cluster ({setting})", fontsize=18)    
    # set axis log
    plt.yscale('log')
    plt.xticks(np.arange(0, max_value_with_margin+1, np.max([1, max_value_with_margin // 10])))
    plt.legend()
    plt.savefig(output_folder+"n_tps_per_cluster.png", bbox_inches='tight', pad_inches=0.2)
    plt.clf()

def hist_total_charge_per_cluster(clusters, output_folder, main_track_label=77, setting="tl_3_cl_1"):
    total_charge = []
    total_charge_supernova = []
    total_charge_mostly_supernova = []
    total_charge_all_supernova = []
    total_charge_main_track = []
    for cluster in clusters:
        total_charge_value = cluster.get_tps()['adc_integral']
        total_charge_value = np.sum(total_charge_value)
        total_charge.append(total_charge_value)
        if cluster.get_supernova_tp_fraction() > 0.:
            total_charge_supernova.append(total_charge_value)
        if cluster.get_supernova_tp_fraction() > 0.5:
            total_charge_mostly_supernova.append(total_charge_value)
        if cluster.get_supernova_tp_fraction() == 1:
            total_charge_all_supernova.append(total_charge_value)
        if cluster.get_true_label() == main_track_label:
            total_charge_main_track.append(total_charge_value)


    # find max value of the histogram
    max_value = max(total_charge)
    max_value_with_margin = int(max_value * 1.1)

    plt.figure(figsize=(11, 7))
    plt.hist(total_charge, bins=50, range=(0, max_value_with_margin+1), label="All clusters", align='left')
    plt.hist(total_charge_supernova, bins=50, range=(0, max_value_with_margin+1), label="Contains supernova clusters", color='red', align='left')
    plt.hist(total_charge_mostly_supernova, bins=50, range=(0, max_value_with_margin+1), label="Mostly supernova clusters", color='orange', align='left')
    plt.hist(total_charge_all_supernova, bins=50, range=(0, max_value_with_margin+1), label="Clean supernova clusters", color='green', align='left')
    # add patch for the longest cluster per event
    plt.hist(total_charge_main_track, bins=50, range=(0, max_value_with_margin+1), label="Main track per event", color='purple', hatch='xx', alpha=0.1, align='left')

    plt.xlabel("Total charge per cluster (adc counts)", fontsize=18)
    plt.ylabel("Number of clusters", fontsize=18)
    plt.title(f"Total charge per cluster ({setting})", fontsize=18)    
    # set axis log
    plt.yscale('log')
    plt.xticks(np.arange(0, max_value_with_margin+1, np.max([1, max_value_with_margin // 10])))
    plt.legend()
    plt.savefig(output_folder+"total_charge_per_cluster.png", bbox_inches='tight', pad_inches=0.2)
    plt.clf()

def hist_max_charge_per_cluster(clusters, output_folder, main_track_label=77, setting="tl_3_cl_1"):
    max_charge = []
    max_charge_supernova = []
    max_charge_mostly_supernova = []
    max_charge_all_supernova = []
    max_charge_main_track = []
    for cluster in clusters:
        max_charge_value = cluster.get_tps()['adc_integral']
        max_charge_value = np.max(max_charge_value)
        max_charge.append(max_charge_value)
        if cluster.get_supernova_tp_fraction() > 0.:
            max_charge_supernova.append(max_charge_value)
        if cluster.get_supernova_tp_fraction() > 0.5:
            max_charge_mostly_supernova.append(max_charge_value)
        if cluster.get_supernova_tp_fraction() == 1:
            max_charge_all_supernova.append(max_charge_value)
        if cluster.get_true_label() == main_track_label:
            max_charge_main_track.append(max_charge_value)

    # find max value of the histogram
    max_value = max(max_charge)
    max_value_with_margin = int(max_value * 1.1)

    plt.figure(figsize=(11, 7))
    plt.hist(max_charge, bins=50, range=(0, max_value_with_margin+1), label="All clusters", align='left')
    plt.hist(max_charge_supernova, bins=50, range=(0, max_value_with_margin+1), label="Contains supernova clusters", color='red', align='left')
    plt.hist(max_charge_mostly_supernova, bins=50, range=(0, max_value_with_margin+1), label="Mostly supernova clusters", color='orange', align='left')
    plt.hist(max_charge_all_supernova, bins=50, range=(0, max_value_with_margin+1), label="Clean supernova clusters", color='green', align='left')
    # add patch for the longest cluster per event
    plt.hist(max_charge_main_track, bins=50, range=(0, max_value_with_margin+1), label="Main track per event", color='purple', hatch='xx', alpha=0.1, align='left')

    plt.xlabel("Max charge per cluster (adc counts)", fontsize=18)
    plt.ylabel("Number of clusters", fontsize=18)
    plt.title(f"Max charge per cluster ({setting})", fontsize=18)    
    # set axis log
    plt.yscale('log')
    plt.xticks(np.arange(0, max_value_with_margin+1, np.max([1, max_value_with_margin // 10])))
    plt.legend()
    plt.savefig(output_folder+"max_charge_per_cluster.png", bbox_inches='tight', pad_inches=0.2)
    plt.clf()

def hist_total_lenght_per_cluster(clusters, output_folder, main_track_label=77, setting="tl_3_cl_1"):
    total_lenght = []
    total_lenght_supernova = []
    total_lenght_mostly_supernova = []
    total_lenght_all_supernova = []
    total_lenght_main_track = []
    for cluster in clusters:
        track_time_lenght = (np.max(cluster.get_tps()['time_start']+cluster.get_tps()['time_over_threshold']) - np.min(cluster.get_tps()['time_start']))*0.08
        track_channel_lenght = (np.max(cluster.get_tps()['channel']) - np.min(cluster.get_tps()['channel']))*0.5
        total_lenght_value = np.sqrt(track_time_lenght**2 + track_channel_lenght**2)

        total_lenght.append(total_lenght_value)
        if cluster.get_supernova_tp_fraction() > 0.:
            total_lenght_supernova.append(total_lenght_value)
        if cluster.get_supernova_tp_fraction() > 0.5:
            total_lenght_mostly_supernova.append(total_lenght_value)
        if cluster.get_supernova_tp_fraction() == 1:
            total_lenght_all_supernova.append(total_lenght_value)
        if cluster.get_true_label() == main_track_label:
            total_lenght_main_track.append(total_lenght_value)

    # find max value of the histogram
    max_value = max(total_lenght)
    max_value_with_margin = int(max_value * 1.1)

    plt.figure(figsize=(11, 7))
    plt.hist(total_lenght, bins=50, range=(0, max_value_with_margin+1), label="All clusters", align='left')
    plt.hist(total_lenght_supernova, bins=50, range=(0, max_value_with_margin+1), label="Contains supernova clusters", color='red', align='left')
    plt.hist(total_lenght_mostly_supernova, bins=50, range=(0, max_value_with_margin+1), label="Mostly supernova clusters", color='orange', align='left')
    plt.hist(total_lenght_all_supernova, bins=50, range=(0, max_value_with_margin+1), label="Clean supernova clusters", color='green', align='left')
    # add patch for the longest cluster per event
    plt.hist(total_lenght_main_track, bins=50, range=(0, max_value_with_margin+1), label="Main track per event", color='purple', hatch='xx', alpha=0.1, align='left')

    plt.xlabel("Total lenght per cluster (cm)", fontsize=18)
    plt.ylabel("Number of clusters", fontsize=18)
    plt.title(f"Total lenght per cluster ({setting})", fontsize=18)
    # set axis log
    plt.yscale('log')
    plt.xticks(np.arange(0, max_value_with_margin+1, np.max([1, max_value_with_margin // 10])))
    plt.legend()
    plt.savefig(output_folder+"total_lenght_per_cluster.png", bbox_inches='tight', pad_inches=0.2)
    plt.clf()

def hist_2D_ntps_total_charge_per_cluster(clusters, output_folder, main_track_label=77, setting="tl_3_cl_1"):
    n_tps = []
    total_charge = []
    for cluster in clusters:
        n_tps.append(len(cluster))
        total_charge_value = cluster.get_tps()['adc_integral']
        total_charge_value = np.sum(total_charge_value)
        total_charge.append(total_charge_value)

    # find max value of the histogram
    max_value_n_tps = max(n_tps)
    max_value_total_charge = max(total_charge)
    max_value_n_tps_with_margin = int(max_value_n_tps * 1.1)
    max_value_total_charge_with_margin = int(max_value_total_charge * 1.1)

    plt.figure(figsize=(11, 7))
    plt.hist2d(n_tps, total_charge, bins=(max_value_n_tps_with_margin+1, 50), range=((0, max_value_n_tps_with_margin), (0, max_value_total_charge_with_margin)), cmap='viridis', cmin=1)
    plt.colorbar(label='Number of clusters')
    plt.xlabel("Number of TPs per cluster", fontsize=18)
    plt.ylabel("Total charge per cluster (adc counts)", fontsize=18)
    plt.title(f"Number of TPs vs Total charge per cluster ({setting})", fontsize=18)
    plt.savefig(output_folder+"n_tps_vs_total_charge_per_cluster.png", bbox_inches='tight', pad_inches=0.2)
    plt.clf()

def hist_n_sn_clusters_per_event(clusters, output_folder, setting="tl_3_cl_1"):
    n_sn_clusters_per_event = {}

    for cluster in clusters:
        event_number = cluster.get_true_event_number()
        if event_number not in n_sn_clusters_per_event:
            n_sn_clusters_per_event[event_number] = 0
        if cluster.get_supernova_tp_fraction() > 0.:
            n_sn_clusters_per_event[event_number] += 1
        
    # find max value of the histogram
    max_value = max(n_sn_clusters_per_event.values())
    max_value_with_margin = int(max_value * 1.1)

    plt.figure(figsize=(11, 7))
    plt.hist(n_sn_clusters_per_event.values(), bins=max_value_with_margin+1, range=(0, max_value_with_margin+1), label="All events", align='left')

    plt.xlabel("Number of supernova clusters per event", fontsize=18)
    plt.ylabel("Number of events", fontsize=18)
    plt.title(f"Number of supernova clusters per event ({setting})", fontsize=18)
    # set axis log
    plt.xticks(np.arange(0, max_value_with_margin+1, np.max([1, max_value_with_margin // 10])))
    plt.legend()
    plt.savefig(output_folder+"n_sn_clusters_per_event.png", bbox_inches='tight', pad_inches=0.2)
    plt.clf()


