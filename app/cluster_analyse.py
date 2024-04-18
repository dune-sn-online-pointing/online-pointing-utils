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

clusters, n_events = read_root_file_to_clusters('/afs/cern.ch/work/h/hakins/private/root_cluster_files/es-cc-bkg-truth/X/uniformBKG/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1.root')
#clusters, n_events = read_root_file_to_clusters('/afs/cern.ch/work/h/hakins/private/online-pointing-utils/scripts/output/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1.root')


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
exlude_expo = [-1,0,1,22] #exlude these bacgkrounds when fitting the exponential function to the histograms
'''
new dic that matches background with its respective maximum decay energy 
format: label: [decay type, max decay energy [MeV], charge of final state nucleus(Z)]
beta = 0
aplha = 1
dont use = 2
'''
background_max_energy = {  
    -1: [2,1],
    0: [2,1 ],
    1: [2,1 ],
    2: [0,.565, 18], #beta
    3: [0,.68706,36], #beta
    4: [0,.59888,18], #beta
    5: [0,3.525516,19], #beta 
    6: [1, 5.59031],  # alpha (MeV)
    7: [1, 6.11468],  # alpha (MeV)
    8: [0, 1.0189,82],   # beta (MeV)
    9: [0, 3.2697,83],   # beta (MeV)
    10: [0, 63.486e-3,82], # beta (MeV)
    11: [0, 1.31107,19], # beta (MeV)
    12: [2,1 ],       # No specified type or energy
    13: [0, .59888,19], # beta (MeV)
    14: [2, 6.11468,84],  # alpha (MeV)  #temporarily disableing this, why would in CPA be different distrigution that LAr
    15: [0, 1.0189,82],   # beta (MeV)
    16: [0, 3.2697,83],   # beta (MeV)
    17: [0, 63.486e-3,82], # beta (MeV)
    18: [0, 1.161292,83],  # beta (MeV)
    19: [0, 2.823067,27], #beta
    20: [2,1 ],
    21: [2,1 ],
    22: [2,1 ]
}
exlude = [] #backgrounds to exlude 19,13,14
charge_lists = {key: [] for key in labels} #lists of sum of charges in each cluster, for histograms
charge_lists_cut = {key: [] for key in labels} #same as above but with a charge cut applied for the purpose of fitting the background
max_charge = {key: 0 for key in labels} #max charge associated with each background, for best fit line
true_labels = []
total_charges = []

apply_cut = True #apply charge cut on 

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
    
    
    
    
    
#manuel overwrite of max charge for outliers
'''
max_charge[2] = 41000
#max_charge[19] = 90000
max_charge[6] = 8000
max_charge[7] = 10000
max_charge[3] = 52000
'''

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
#plt.savefig('plots/backgrounds/true_labels.png')
plt.savefig('plots/backgrounds_uniform/true_labels.png')





#plot every background and find max ADC values to fit beta spectrum to

beta_adc = []
beta_adc_error = []
beta_MeV = []
alpha_adc = []
alpha_adc_error = []
alpha_MeV = []

for label_num, charge_list in charge_lists.items():
    
    energy_list = [] #new list with converted adc to energy values
    for adc_charge in charge_list:
        energy_list.append(adc_to_energy(adc_charge))
    max_charge_num = 0
    
    
    
    #histogram
    fig1 = plt.figure()
    hist, bins, _ = plt.hist(charge_list, bins=200)  
    plt.title(f'Total Charge per {labels[label_num]} Cluster [ADC]')  
    plt.xlabel('Total Charge per Cluster [ADC]')  
    plt.ylabel('Number of Clusters')  
    #fig1.savefig(f'plots/backgrounds/{labels[label_num]}_ADC.png')
    fig1.savefig(f'plots/backgrounds_uniform/{labels[label_num]}_ADC.png')

    plt.clf()
    
    
    # Fit a polynomial to the histogram data

    fig2 = plt.figure()
    hist, bins, _ = plt.hist(charge_list, bins=200)  
    plt.title(f'Total Charge per {labels[label_num]} Cluster [ADC]')  
    plt.xlabel('Total Charge per Cluster [ADC]')  
    plt.ylabel('Number of Clusters') 
    
    if background_max_energy[label_num][0] == 0 and label_num not in exlude:
        beta_adc_error.append(np.std(hist))
        
    elif background_max_energy[label_num][0] ==1 and label_num not in exlude:
        alpha_adc_error.append(np.std(hist))


    degree = 5  
    coefficients = np.polyfit((bins[:-1] + bins[1:]) / 2, hist, degree)
    poly = np.poly1d(coefficients)
    zeros = np.roots(coefficients)
    

    # Plot the fitted curve
    x_values = np.linspace(bins.min(), bins.max(), 1000)
    plt.plot(x_values, poly(x_values), 'r-', label='Fit')
    plt.show()
    # Save the plot
    equation = str(poly)
    plt.text(0.5, 0.9, equation, horizontalalignment='center', verticalalignment='center', transform=plt.gca().transAxes, fontsize=7)
    #fig2.savefig(f'plots/polynomial_fits/{labels[label_num]}_ADC.png')
    fig2.savefig(f'plots/polynomial_uniform/{labels[label_num]}_ADC.png')


    #    # Print the equation of the fitted polynomial

    
    plt.clf()
    
    
    #fit exponential to hist data


    if label_num not in exlude_expo:
        cut = np.std(charge_list)
        cut_charge_list = [x for x in charge_list if x >= cut] # charge cut to cut out spike at low energies
        fig_expo = plt.figure()
        hist, bins, _ = plt.hist(cut_charge_list, bins=60)  
        plt.title(f'Total Charge per {labels[label_num]} Cluster [ADC]')  
        plt.xlabel('Total Charge per Cluster [ADC]')  
        plt.ylabel('Number of Clusters') 
        
        def sigmoid(x, a, b, c, d):
            return a / (1 + np.exp(-b * (x - c))) + d

        x = (bins[:-1] + bins[1:]) / 2  # Get x values for the center of each bin
        params, pcov = curve_fit(sigmoid, x, hist, p0=[-hist[0], 1/np.std(hist), np.mean(hist), 100])
        #print("Background: " + str(labels[label_num]))
        #print("Parameters: " + str(params))
        #print("hist[0]: " + str(hist[0]))
        #print("STD: " + str(np.std(hist)))
        #print("Mean: " + str(np.mean(hist)))
        # Step 5: Plot fitted sigmoid curve
        x_curve = np.linspace(min(x), max(x), 100)
        plt.plot(x_curve, sigmoid(x_curve, *params), 'r-', label='Sigmoid Fit')
        plt.text(0.65, 0.9, f'{round(params[0],2)}/[1+exp({round(params[1],2)} * (x-{round(params[2],2)}))] + {round(params[3],2)}', horizontalalignment='center', verticalalignment='center', transform=plt.gca().transAxes, fontsize=7)

        plt.show()
        #fig_expo.savefig(f'plots/exponential_fits/{labels[label_num]}_ADC.png')
        fig_expo.savefig(f'plots/exponential_uniform/{labels[label_num]}_ADC.png')

        plt.clf()
        
        #Fit Fermi Function
        alpha = .007297
        m = .511 #MeV
        c = 1   #m/s
        e = np.sqrt(4*np.pi*alpha) #C
        h = 1 # Joules-s / rad
        
        def E(T): #total energy
            return T + np.power(m,2)
        def P(T): #momentum
            return np.sqrt(np.power(E(T),2) - np.power(m,2))
        
        def eta(Z,T): #n
            return (Z*E(T)*np.power(e,2))/P(T)
        
        def F(Z,T): #fermi function
            numerator = (2*np.pi*eta(Z,T))
            denominator = (1-np.exp(-2*np.pi*eta(Z,T)))  
            return numerator/denominator    
        
        def N(T,Z,C,Q):
            return C*F(Z,T)*P(T)*E(T)*np.power((Q-T),2)
        
    if (label_num not in exlude_expo) and (len(background_max_energy[label_num]) > 2):
        scale = 100000
        cut = (max(charge_list)/scale)/25 #cut out first 1st 1/25th of hist range
        # to fit the expression for beta decay onto the ADC graph, I had to first modify the histogram because the expression does not scale
        # Now just find endpoint, and reverse the modification to find on ADC graph 
        # i.e. Find endpoint of fit, subtract .25, then multiply by 100,000 to find ADC endpoint
        cut_charge_list = [x/100000 + .25 for x in charge_list if x/100000 > cut] # charge cut to cut out spike at low energies
        fig_fermi = plt.figure()
        hist, bins, _ = plt.hist(cut_charge_list, bins=50)  
        plt.title(f'Total Charge per {labels[label_num]} Cluster [ADC]')  
        plt.xlabel('Total Charge per Cluster [ADC]')  
        plt.ylabel('Number of Clusters') 
        x = (bins[:-1] + bins[1:]) / 2  # Get x values for the center of each bin
        params, pcov = curve_fit(N, x, hist, p0=[background_max_energy[label_num][2],4,background_max_energy[label_num][1]])
        x_curve = np.linspace(min(x), max(x), 100)
        plt.plot(x_curve, N(x_curve, *params), 'r-', label='Beta Fit')
        plt.show()
        #fig_fermi.savefig(f'plots/with_fermi/{labels[label_num]}_ADC.png')
        fig_fermi.savefig(f'plots/with_fermi_uniform/{labels[label_num]}_ADC.png')
        plt.clf()
        
        print("Background: " + str(labels[label_num]))
        print("Background Z,Q: " + str(background_max_energy[label_num][2]) + " , " + str(background_max_energy[label_num][1]))
        print("Params [Z,C,Q]: " + str(params))
    
    
    
    '''
    fig1 = plt.figure()
    hist, bins, _ = plt.hist(charge_list, bins=200)  
    plt.title(f'Total Charge per {labels[label_num]} Cluster [ADC]')  
    plt.xlabel('Total Charge per Cluster [ADC]')  
    plt.ylabel('Number of Clusters')  
    plt.show()
    
    
    fig1.savefig(f'plots/tests/{labels[label_num]}_ADC_Sonk.png')
    plt.clf()
    
    
    
    
     
    for i in range(len(hist)):
        if (bins[i] > max_charge and hist[i] > 1) : #max ADC must have more than 3 entires, filter out the outliers
            max_charge = bins[i]
    # Save the plot
   
    
    
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
#fig4.savefig('plots/backgrounds/total_charge.png')
fig4.savefig('plots/backgrounds_uniform/total_charge.png')





#create two lists for beta and alpha decay background endpoints for linear fit

for key in background_max_energy:
    print("Background: " + labels[key] + " Max ADC: " + str(max_charge[key]))
    if key not in exlude:
        if background_max_energy[key][0] == 0:
            beta_adc.append(max_charge[key])
            beta_MeV.append(background_max_energy[key][1])
        elif background_max_energy[key][0] == 1:
            alpha_adc.append(max_charge[key])
            alpha_MeV.append(background_max_energy[key][1])


#linear fit
#error bars on points, use sigma of histogram

slope_B, intercept_B, r_value_B, p_value_B, std_err_B = linregress(beta_adc, beta_MeV)
slope_a, intercept_a, r_value_a, p_value_a, std_err_a = linregress(alpha_adc, alpha_MeV)

fig0 = plt.figure()

beta_fit_line = f'Beta Fit: MeV = {slope_B:.2e} * ADC + {intercept_B:.2f} (R²={r_value_B**2:.2f})'
alpha_fit_line = f'Alpha Fit: MeV = {slope_a:.2e} * ADC + {intercept_a:.2f} (R²={r_value_a**2:.2f})'

print("Beta slope: " + str(slope_B))
print("Alpha slope: " + str(slope_a))

plt.scatter(beta_adc, beta_MeV, color = 'red', label='Beta Decay Backgrounds')
#plt.errorbar(beta_adc,beta_MeV,xerr=beta_adc_error)
plt.scatter(alpha_adc,alpha_MeV, color = 'blue', label='Alpha Decay Backgrounds')
#plt.errorbar(alpha_adc,alpha_MeV,xerr=alpha_adc_error)
plt.plot(beta_adc, [slope_B * x + intercept_B for x in beta_adc], color='red', label=beta_fit_line)
plt.plot(alpha_adc,[slope_a * x + intercept_a for x in alpha_adc], color='blue', label=alpha_fit_line)

exclude_str = ', '.join(str(labels[val]) for val in exlude)

# Create the plot title using the formatted string
plt.title(f'ADC to MeV excluding {exclude_str}')
plt.xlabel('ADC')
plt.ylabel('MeV')
plt.legend()
#fig0.savefig('plots/ADC_KeV_fits/beta_fit_raw_endpoints.png')
fig0.savefig('plots/ADC_KeV_uniform/beta_fit_raw_endpoints.png')

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