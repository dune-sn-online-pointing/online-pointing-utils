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
from scipy.stats import linregress
from scipy.optimize import curve_fit
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
    5: "kK42From42ArGenInLAr", #beta  3.525516MeV
    6: "kRn222ChainRn222GenInLAr", #alpha 5.59031MeV
    7: "kRn222ChainPo218GenInLAr", #alpha 6.11468MeV
    8: "kRn222ChainPb214GenInLAr", #beta 1.0189 MeV
    9: "kRn222ChainBi214GenInLAr",  #beta 3.2697MeV
    10: "kRn222ChainPb210GenInLAr", #beta 63.486keV
    11: "kK40GenInCPA", #89.28%	β-	1.31107MeV	40Ca ------ 10.72%	β+	482.491keV	40Ar
    12: "kU238ChainGenInCPA",
    13: "kK42From42ArGenInCPA", #beta 598.88keV
    14: "kRn222ChainPo218GenInCPA",#alpha .02%beta  6.11468MeV
    15: "kRn222ChainPb214GenInCPA",#beta 1.0189 MeV
    16: "kRn222ChainBi214GenInCPA", #beta 3.2697MeV
    17: "kRn222ChainPb210GenInCPA", #beta 63.486keV
    18: "kRn222ChainFromBi210GenInCPA", #beta 1.161292MeV
    19: "kCo60GenInAPA", #beta 2.823067MeV
    20: "kU238ChainGenInAPA",
    21: "kRn222ChainGenInPDS",
    22: "kNeutronGenInRock"
    }

'''
new dic that matches background with its respective maximum decay energy 
format: label: [decay type, decay energy [MeV]]
beta = 0
aplha = 1
dont use = 2
'''
background_max_energy = {  
    -1: [2,1],
    0: [2,1 ],
    1: [2,1 ],
    2: [0,.565], #beta
    3: [0,.68706], #beta
    4: [0,.59888], #beta
    5: [0,3.525516], #beta 
    6: [1, 5.59031],  # alpha (MeV)
    7: [1, 6.11468],  # alpha (MeV)
    8: [0, 1.0189],   # beta (MeV)
    9: [0, 3.2697],   # beta (MeV)
    10: [0, 63.486e-3], # beta (MeV)
    11: [0, 1.31107], # beta (MeV)
    12: [2,1 ],       # No specified type or energy
    13: [0, .59888], # beta (MeV)
    14: [1, 6.11468],  # alpha (MeV)
    15: [0, 1.0189],   # beta (MeV)
    16: [0, 3.2697],   # beta (MeV)
    17: [0, 63.486e-3], # beta (MeV)
    18: [0, 1.161292],  # beta (MeV)
    19: [0, 2.823067], #beta
    20: [2,1 ],
    21: [2,1 ],
    22: [2,1 ]
}
exlude = [19,13,] #backgrounds to exlude
charge_lists = {key: [] for key in labels} #lists of sum of charges in each cluster, for histograms
max_charge = {key: 0 for key in labels} #max charge associated with each background, for best fit line
true_labels = []
total_charges = []

for cluster in clusters:
    charge_sums = {key: 0 for key in labels}
    total_charge_sum = 0
    
    #summing ADC integral for each TP set in cluster
    for tp in cluster.tps_:
        charge_sums[cluster.true_label_] += tp['adc_integral']
        total_charge_sum = total_charge_sum + tp['adc_integral']
    
    #checking to see if new cluster charge is max    
    if (charge_sums[cluster.true_label_] > max_charge[cluster.true_label_]):
        max_charge[cluster.true_label_] = charge_sums[cluster.true_label_] 

    charge_lists[cluster.true_label_].append(charge_sums[cluster.true_label_])
    true_labels.append(cluster.true_label_)
    total_charges.append(total_charge_sum)
    
        

#CONVERT ADC to Energy (KeV) via linear interpolation


#decay energies from https://periodictable.com/Isotopes/018.39/index.dm.html

#point 1 is endpoint of Ar39 decay
x1 = 41100 #ADC
y1 = 565 #KeV  
#point 2 is endpoint of Kr85 decay
x2 = 50000 #ADC
y2 = 687.06 #KeV

m = (y2-y1)/(x2-x1) #slope 

def adc_to_energy(adc):
    return m*(adc-x1)+y1




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



#plot every background and find max ADC values to fit beta spectrum to

beta_adc = []
beta_MeV = []
alpha_adc = []
alpha_MeV = []

for label_num, charge_list in charge_lists.items():
    energy_list = [] #new list with converted adc to energy values
    for adc_charge in charge_list:
        energy_list.append(adc_to_energy(adc_charge))
    max_charge = 0
    fig1 = plt.figure()
    hist, bins, _ = plt.hist(charge_list, bins=200)  
    plt.title(f'Total Charge per {labels[label_num]} Cluster [ADC]')  
    plt.xlabel('Total Charge per Cluster [ADC]')  
    plt.ylabel('Number of Clusters')  
    def gaussian(x, amplitude, mean, stddev):
        return amplitude * np.exp(-((x - mean) / stddev) ** 2 / 2)

    # Find optimal parameters for the fit
    bin_centers = (bins[:-1] + bins[1:]) / 2
    popt, pcov = curve_fit(gaussian, bin_centers, hist, p0=[1, np.mean(charge_list), np.std(charge_list)])

    # Plot the fitted curve
    plt.plot(bin_centers, gaussian(bin_centers, *popt), 'r-', label='Fit')

    # Save the plot
    fig1.savefig(f'plots/test/{labels[label_num]}_ADC_with_fit.png')

    # Show the plot
    plt.legend()
    plt.show()
    plt.clf()    
    
    
    
    '''
    for i in range(len(hist)):
        if (bins[i] > max_charge and hist[i] > 1) : #max ADC must have more than 3 entires, filter out the outliers
            max_charge = bins[i]
    # Save the plot
    fig1.savefig(f'plots/tests/{labels[label_num]}_ADC_Sonk.png')
    plt.clf()
    
    if background_max_energy[label_num][0] == 0:
        beta_adc.append(max_charge)
        beta_MeV.append(background_max_energy[label_num][1])
    elif background_max_energy[label_num][0] == 1:
        alpha_adc.append(max_charge)
        alpha_MeV.append(background_max_energy[label_num][1])
        
    '''  
    
    
    '''
    fig1 = plt.figure()
    plt.hist(charge_list, bins=200)  
    plt.title(f'Total Charge per {labels[label_num]} Cluster [ADC]')  
    plt.xlabel('Total Charge per Cluster [ADC]')  
    plt.ylabel('Number of Clusters')  
    #plt.yscale("log")
    fig1.savefig(f'plots/{labels[label_num]}_ADC.png')
    plt.clf()
    
    fig2 = plt.figure()
    plt.hist(energy_list, bins=200)  
    plt.title(f'Total Charge per {labels[label_num]} Cluster [KeV]')  
    plt.xlabel('Total Charge per Cluster [KeV]')  
    plt.ylabel('Number of Clusters')  
    #plt.yscale("log")
    fig2.savefig(f'plots/{labels[label_num]}_KeV.png')
    plt.clf()
    '''

fig4 = plt.figure()
plt.hist(total_charges, bins = 100)
plt.ylabel("Number of Clusters")
plt.xlabel("Total Charge per Cluster [ADC]")
plt.yscale("log")
plt.title("Cluster Charge for all Events")
fig4.savefig('plots/total_charge.png')




#create two lists for beta and alpha decay background endpoints for linear fit
'''
for key in background_max_energy:
    if key not in exlude:
        print("Key: " +str(key))
        print("Max charge: " + str(max_charge[key]))
        if background_max_energy[key][0] == 0:
            beta_adc.append(max_charge[key])
            beta_MeV.append(background_max_energy[key][1])
        elif background_max_energy[key][0] == 1:
            alpha_adc.append(max_charge[key])
            alpha_MeV.append(background_max_energy[key][1])
''' 

#linear fit
#error bars on points, use sigma of histogram

slope_B, intercept_B, r_value_B, p_value_B, std_err_B = linregress(beta_adc, beta_MeV)
slope_a, intercept_a, r_value_a, p_value_a, std_err_a = linregress(alpha_adc, alpha_MeV)

fig0 = plt.figure()

beta_fit_line = f'Beta Fit: MeV = {slope_B:.2f} * ADC + {intercept_B:.2f} (R²={r_value_B**2:.2f})'
alpha_fit_line = f'Alpha Fit: MeV = {slope_a:.2f} * ADC + {intercept_a:.2f} (R²={r_value_a**2:.2f})'

print("Beta slope: " + str(slope_B))
print("Alpha slope: " + str(slope_a))

plt.scatter(beta_adc, beta_MeV, color = 'red', label='Beta Decay Backgrounds')
plt.scatter(alpha_adc,alpha_MeV, color = 'blue', label='Alpha Decay Backgrounds')
plt.plot(beta_adc, [slope_B * x + intercept_B for x in beta_adc], color='red', label=beta_fit_line)
plt.plot(alpha_adc,[slope_a * x + intercept_a for x in alpha_adc], color='blue', label=alpha_fit_line)
plt.xlabel('ADC')
plt.ylabel('MeV')
plt.legend()
fig0.savefig('plots/tests/beta_fit2.png')
plt.clf()







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