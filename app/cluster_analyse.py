"""
 * @file create_image.py
 * @brief Creates 2D images of TPs, plotting channel vs time. Colors represent the charge of the TP.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2024.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
"""

import numpy as np
#import ROOT
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

from cluster import read_root_file_to_clusters

# parser for the arguments
parser = argparse.ArgumentParser(description='Tranforms Trigger Primitives to images.')
parser.add_argument('-i', '--input-file',           type=str,                         help='Input file name')
parser.add_argument('-o', '--output-path',          type=str,    default='./',        help='path to save the image')
parser.add_argument('-n', '--n-tps',                type=int,    default=100,           help='number of tps to process')
parser.add_argument('-t', '--ticks-limit',          type=int,    default=3,           help='closeness in ticks to cluster TPs')
parser.add_argument('-c', '--channel-limit',        type=int,    default=1,           help='closeness in channels to cluster TPs')
parser.add_argument('-m', '--min-tps',              type=int,    default=3,           help='minimum number of TPs to create a cluster and then an image')
parser.add_argument('--channel-map',                type=str,    default="APA",       help='"APA", "CRP" or "50L"')
parser.add_argument('--save-clusters',                action='store_true',     default=True,          help='write the clusters to a textfile')
parser.add_argument('--show',                       action='store_true',     default=True,          help='show the image')
parser.add_argument('--fixed-size',                 action='store_true',     default=True,          help='make the image size fixed')
parser.add_argument('--img-width',                  type=int,    default=200,          help='width of the image, if fixed size')
parser.add_argument('--img-height',                 type=int,    default=300,         help='height of the image, if fixed size')
parser.add_argument('--x-margin',                   type=int,    default=2,           help='margin in x')
parser.add_argument('--y-margin',                   type=int,    default=2,          help='margin in y')
parser.add_argument('-v', '--verbose',              action='store_true', default=True,             help='verbose mode')

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

# if verbose:
#     print("TPs read from file: ", input_file)
#     print("Number of TPs: ", len(all_TPs))
#     print(" ")

# create channel map to distinguish induction and collection
# my_channel_map = create_channel_map_array(which_channel_map=channel_map)

clusters, n_events = read_root_file_to_clusters('/afs/cern.ch/work/h/hakins/private/online-pointing-utils/scripts/output/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1.root')

print("Number of clusters: ", len(clusters))
#print("First cluster entry: " + str(clusters[0]))

#background labels
labels = {
   -1: "multilabel",
    0: "kUnknown",
    1: "kMarley",
    2: "kAr39GenInLAr", #beta
    3: "kKr85GenInLAr", #beta
    4: "kAr42GenInLAr", #beta
    5: "kK42From42ArGenInLAr", #beta 
    6: "kRn222ChainRn222GenInLAr", #alpha 5.59031MeV
    7: "kRn222ChainPo218GenInLAr", #alpha .02%beta  6.11468MeV
    8: "kRn222ChainPb214GenInLAr", #beta 1.0189 MeV
    9: "kRn222ChainBi214GenInLAr",  #https://periodictable.com/Isotopes/083.214/index.full.dm.html
    10: "kRn222ChainPb210GenInLAr",
    11: "kK40GenInCPA",
    12: "kU238ChainGenInCPA",
    13: "kK42From42ArGenInCPA",
    14: "kRn222ChainPo218GenInCPA",
    15: "kRn222ChainPb214GenInCPA",
    16: "kRn222ChainBi214GenInCPA",
    17: "kRn222ChainPb210GenInCPA",
    18: "kRn222ChainFromBi210GenInCPA",
    19: "kCo60GenInAPA",
    20: "kU238ChainGenInAPA",
    21: "kRn222ChainGenInPDS",
    22: "kNeutronGenInRock"
    }

charge_lists = {key: [] for key in labels}
true_labels = []
total_charges = []
for cluster in clusters:
    charge_sums = {key: 0 for key in labels}
    total_charge_sum = 0
    for tp in cluster.tps_:
        charge_sums[cluster.true_label_] += tp['adc_integral']
        total_charge_sum = total_charge_sum + tp['adc_integral']

    charge_lists[cluster.true_label_].append(charge_sums[cluster.true_label_])
    true_labels.append(cluster.true_label_)
    total_charges.append(total_charge_sum)
    
        


#CONVERT ADC to Energy (KeV) via linear interpolation


#energies from https://periodictable.com/Isotopes/018.39/index.dm.html
#point 1 is endpoint of Ar39 decay
x1 = 41100 #ADC
y1 = 565 #KeV  
#point 2 is endpoint of Kr85 decay
x2 = 50000 #ADC
y2 = 687.06 #KeV

m = (y2-y1)/(x2-x1)

def adc_to_energy(adc):
    return m*(adc-x1)+y1



'''
SN = 1
Ar39LAr = 2
Kr85LAr = 3

total_charges = [] #array of total charges per cluster
true_labels = []
Ar39LAr_charges = []
SN_charges = []


for cluster in clusters:
    charge_sum = 0
    Ar39_charge_sum = 0
    SN_charge_sum = 0

    if (cluster.true_label_ == Ar39LAr):
        for tp in cluster.tps_:
            Ar39_charge_sum = Ar39_charge_sum + tp['adc_integral']
        Ar39LAr_charges.append(Ar39_charge_sum)

    if (cluster.true_label_ == SN):
        for tp in cluster.tps_:
            #print("Supernova Label Identified. TP charge: " + str(tp['adc_integral']))
            #print("SN charge sum before adding: " + str(SN_charge_sum))
            SN_charge_sum = SN_charge_sum + tp['adc_integral']
            #print("SN charge sum just added: " + str(SN_charge_sum))
        SN_charges.append(SN_charge_sum)

    for tp in cluster.tps_:
        charge_sum = charge_sum + tp['adc_integral']
    

    true_labels.append(cluster.true_label_)
    total_charges.append(charge_sum)
'''





# Add labels beside the graph
bin_edges = [i - 0.5 for i in range(min(true_labels), max(true_labels) + 2)]


colors = ['blue', 'orange', 'green', 'red', 'black', 'blue', 'orange', 'gray', 'blue', 'green', 'black', 'yellow', 'black', 'blue', 'orange', 'green', 'red', 'blue', 'brown', 'green', 'gray', 'olive', 'yellow']






# histogram with custom colors
n, bins, patches = plt.hist(true_labels, bins = bin_edges, align = "mid")
for c, p in zip(colors, patches):
    p.set_facecolor(c)

plt.ylabel("Number of Clusters")
plt.xlabel("Backgrounds")
plt.yscale("log")
plt.xticks(range(min(true_labels), max(true_labels) + 1))
plt.title("Frequency of Different Background Events")
plt.savefig('plots/true_labels.png')
#plot every background


for label_num, charge_list in charge_lists.items():
    energy_list = [] #new list with converted adc to energy values
    for adc_charge in charge_list:
        energy_list.append(adc_to_energy(adc_charge))
    fig1 = plt.figure()
    plt.hist(charge_list, bins=300)  
    plt.title(f'Total Charge per {labels[label_num]} Cluster [ADC]')  
    plt.xlabel('Total Charge per Cluster [ADC]')  
    plt.ylabel('Number of Clusters')  
    #plt.yscale("log")
    fig1.savefig(f'plots/{labels[label_num]}_ADC.png')
    plt.clf()
    
    fig2 = plt.figure()
    plt.hist(energy_list, bins=300)  
    plt.title(f'Total Charge per {labels[label_num]} Cluster [KeV]')  
    plt.xlabel('Total Charge per Cluster [KeV]')  
    plt.ylabel('Number of Clusters')  
    #plt.yscale("log")
    fig2.savefig(f'plots/{labels[label_num]}_KeV.png')
    plt.clf()
    

fig4 = plt.figure()
plt.hist(total_charges, bins = 100)
plt.ylabel("Number of Clusters")
plt.xlabel("Total Charge per Cluster [ADC]")
plt.yscale("log")
plt.title("Cluster Charge for all Events")
fig4.savefig('plots/total_charge.png')

'''
# Plot 2: Ar39LAr # groups vs charge/group
fig2 = plt.figure()
plt.hist(Ar39LAr_charges,bins = 300)
plt.xlabel("Total Charge per Ar39LAr Cluster [ADC]")
plt.ylabel('Number of Clusters')
plt.xlim(0,40000)
plt.title("Total Charge per Ar39LAr Event Cluster ")
fig2.savefig('plots/Ar39LAr_charge.png')


# Plot 2: SN # groups vs charge/group
fig3 = plt.figure()
plt.hist(SN_charges,bins = 3000)
plt.xlabel("Total Charge per SN Cluster [ADC]")
plt.ylabel('Number of Clusters')
plt.xlim(0,140000)
plt.yscale("log")
plt.title("Total Charge per SN Event Cluster [ADC]")

fig3.savefig('plots/SN_charge.png')
'''

#copy plot to local: scp hakins@lxplus.cern.ch:/afs/cern.ch/user/h/hakins/tpgtools/scripts/plots/histogram.png C:\Users\harry\Documents\Test_Plots