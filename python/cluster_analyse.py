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
from sklearn.metrics import r2_score
from scipy.special import gamma
from scipy.stats import chisquare
import random

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

#True means you want to use uniform background file, changing where images are saved etc..
uniform = True

if uniform:
    #clusters, n_events = read_root_file_to_clusters('/afs/cern.ch/work/h/hakins/private/root_cluster_files/es-cc-bkg-truth/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1.root') #uniform backgrounds updated
    clusters, n_events = read_root_file_to_clusters('/afs/cern.ch/work/h/hakins/private/root_cluster_files/es-cc-bkg-truth/X/uniformBKG/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1.root') #uniform backgrounds with half missing
else:
    clusters, n_events = read_root_file_to_clusters('/afs/cern.ch/work/h/hakins/private/online-pointing-utils/scripts/output/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1.root')

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
format: label: [decay type, max decay energy [MeV], charge of final state nucleus(Z), uncertainty in endpoint,radius of nucleus]
beta = 0
aplha = 1
dont use = 2
'''

counterr=0
#decay energies from https://periodictable.com/Isotopes/018.39/index.dm.html
background_max_energy = {  
    -1: [2,1,0,0],
    0: [2,1,0,0 ],
    1: [2,1,0,0 ],
    2: [0,.565, 18,0,0], #beta
    3: [0,.68706,36,0,0], #beta
    4: [0,.59888,18,0,0], #beta
    5: [0,3.525516,19,0,0], #beta 
    6: [1, 5.59031,0,0],  # alpha (MeV)
    7: [1, 6.11468,0,0],  # alpha (MeV)
    8: [0, 1.0189,82,0,0],   # beta (MeV)
    9: [0, 3.2697,83,0,0],   # beta (MeV)
    10: [0, 63.486e-3,82,0,0], # beta (MeV)
    11: [0, 1.31107,19,0,0], # beta (MeV)
    12: [2,1,0,0 ],       # No specified type or energy
    13: [0, .59888,19,0,0], # beta (MeV)
    14: [2, 6.11468,84,0,0],  # alpha (MeV)  #temporarily disableing this, why would in CPA be different distrigution that LAr
    15: [0, 1.0189,82,0],   # beta (MeV)
    16: [0, 3.2697,83,0],   # beta (MeV)
    17: [0, 63.486e-3,82,0], # beta (MeV)
    18: [0, 1.161292,83,0],  # beta (MeV)
    19: [0, 2.823067,27,0], #beta
    20: [2,1,0,0 ],
    21: [2,1,0,0 ],
    22: [2,1,0,0]
}
#returns recovered area (not including adc_integral)
threshold=30
def recover_area(hit): 
    x1,y1,x2,y2= hit['time_start'],threshold,hit['time_peak']+hit['time_start'],hit['adc_peak'] 
    slope = (y2-y1)/(x2-x1)
    start_x =(0-y1)/slope + x1
    end_x = x1
    
    x_values = np.linspace(start_x, end_x, 100)
    #y_values = slope * (x_values - x1) + y1
    area_left = (np.abs(start_x-end_x)*threshold)/2  #formula for triangle area

    #Find Equation of Negative Slope Line (Right Side)
    x1,y1,x2,y2= hit['time_start']+hit['time_over_threshold']-1,threshold,hit['time_peak']+hit['time_start'],hit['adc_peak']
    slope = (y2-y1)/(x2-x1)
    start_x = x1
    end_x = (0-y1)/slope + x1
    x_values = np.linspace(start_x, end_x, 100)
    #y_values = slope * (x_values - x1) + y1
    area_right = (np.abs(start_x-end_x)*threshold)/2  #formula for triangle area
    if hit['adc_integral'] > 2000: #5500 with 19 bins best so far
        return hit['adc_integral']
    total_area = area_right+area_left+hit['adc_integral']
    if total_area>4*hit["adc_integral"]:
        total_area=hit['adc_integral']/.6
    return total_area 


def recover_formula(hit):
    adc = hit['adc_integral']
    true_area = .97*adc + 113.23
    return true_area
    
if uniform:
    exlude = [19,11,13,16,17,18,10,15,14] 
else:
    exlude = [19,13,14,2] #backgrounds to exlude #exluding Ar39 because Fermi fits badly with outlier

exlude_expo = []#[-1,0,1,22,2] #exlude these bacgkrounds when fitting the exponential function to the histograms

     
charge_lists = {key: [] for key in labels} #lists of sum of charges in each cluster, for histograms
charge_lists_cut = {key: [] for key in labels} #same as above but with a charge cut applied for the purpose of fitting the background
max_charge = {key: 0 for key in labels} #max charge associated with each background, for best fit line using max adc
max_charge_fermi = {key: 0 for key in labels} #max charge using fermi fit
max_charge_fermi_r = {key: 0 for key in labels} 
true_labels = []
total_charges = []
charge_lists_recovered = {key: [] for key in labels}
charge_lists_recovered_formula = {key: [] for key in labels} #recovery using log fit to fraction vs adc integral plot


for cluster in clusters:
    charge_sums = {key: 0 for key in labels}
    charge_sums_recovered = {key: 0 for key in labels}
    charge_sums_recovered_formula = {key: 0 for key in labels}
    total_charge_sum = 0
    
    #summing ADC integral for each TP set in cluster
    for tp in cluster.tps_:
        charge_sums_recovered[cluster.true_label_] += recover_area(tp)
        charge_sums_recovered_formula[cluster.true_label_] += recover_formula(tp)
        charge_sums[cluster.true_label_] += tp['adc_integral']
        total_charge_sum = total_charge_sum + tp['adc_integral']
    
    #checking to see if new cluster charge is max    
    if (charge_sums[cluster.true_label_] > max_charge[cluster.true_label_]):
        max_charge[cluster.true_label_] = charge_sums[cluster.true_label_] 

    charge_lists[cluster.true_label_].append(charge_sums[cluster.true_label_])
    charge_lists_recovered[cluster.true_label_].append(charge_sums_recovered[cluster.true_label_])
    charge_lists_recovered_formula[cluster.true_label_].append(charge_sums_recovered_formula[cluster.true_label_])
    true_labels.append(cluster.true_label_)
    total_charges.append(total_charge_sum)
    
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
if uniform:
    plt.savefig('plots/backgrounds_uniform/true_labels.png')
else:
    plt.savefig('plots/backgrounds/true_labels.png')

#plot every background and find max ADC values to fit beta spectrum to
beta_adc = []
beta_adc_error = []
beta_MeV = []
beta_MeV.append(0)
alpha_adc = []
alpha_adc_error = []
alpha_MeV = []

exit()
for label_num, charge_list in charge_lists.items():
    bins = 28
    
    max_charge_num = 0   
    fig2=plt.figure()
    hist, bins, _ = plt.hist(charge_lists_recovered[label_num], bins=bins)  
    plt.title(f'Total Charge per {labels[label_num]} Cluster [ADC]')  
    plt.xlabel('Total Charge per Cluster [ADC]')  
    plt.ylabel('Number of Clusters')  
    fig2.savefig(f'plots/backgrounds_recovered/{labels[label_num]}_ADC_.png')
    plt.clf()
    plt.close()
    
    fig3=plt.figure()
    hist, bins, _ = plt.hist(charge_lists_recovered_formula[label_num], bins=28)  
    plt.title(f'Total Charge per {labels[label_num]} Cluster [ADC]')  
    plt.xlabel('Total Charge per Cluster [ADC]')  
    plt.ylabel('Number of Clusters')  
    fig3.savefig(f'plots/backgrounds_recovered_formula/{labels[label_num]}_ADC_.png')
    plt.clf()
    plt.close()
    
    max_charge_num = 0   
    #histogram of each backgrouund
    fig1 = plt.figure()
    hist, bins, _ = plt.hist(charge_list, bins=bins)  
    plt.title(f'Total Charge per {labels[label_num]} Cluster [ADC]')  
    plt.xlabel('Total Charge per Cluster [ADC]')  
    plt.ylabel('Number of Clusters')  
    if uniform:
        fig1.savefig(f'plots/backgrounds_uniform/{labels[label_num]}_ADC.png')
    else:
        fig1.savefig(f'plots/backgrounds/{labels[label_num]}_ADC.png')
    plt.clf()
    plt.close()

exit()
for label_num, charge_list in charge_lists.items():
    max_charge_num = 0   
    #histogram of each backgrouund
    fig1 = plt.figure()
    hist, bins, _ = plt.hist(charge_list, bins=27)  
    plt.title(f'Total Charge per {labels[label_num]} Cluster [ADC]')  
    plt.xlabel('Total Charge per Cluster [ADC]')  
    plt.ylabel('Number of Clusters')  
    if uniform:
        fig1.savefig(f'plots/backgrounds_uniform/{labels[label_num]}_ADC.png')
    else:
        fig1.savefig(f'plots/backgrounds/{labels[label_num]}_ADC.png')
    plt.clf()
    plt.close()
    fig2=plt.figure()
    hist, bins, _ = plt.hist(charge_lists_recovered[label_num], bins=30)  
    plt.title(f'Total Charge per {labels[label_num]} Cluster [ADC]')  
    plt.xlabel('Total Charge per Cluster [ADC]')  
    plt.ylabel('Number of Clusters')  
    fig2.savefig(f'plots/backgrounds_recovered/{labels[label_num]}_ADC_.png')
    plt.clf()
    plt.close()
    
    
    # Fit a polynomial to the histogram data
    '''
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
    if uniform:
        fig2.savefig(f'plots/polynomial_uniform/{labels[label_num]}_ADC.png')
    else:
        fig2.savefig(f'plots/polynomial_fits/{labels[label_num]}_ADC.png')
   
    plt.clf()
    
  '''
  
    #fit sigmoid to hist data
 
    if label_num not in exlude_expo:
        '''
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
        try:
            params, pcov = curve_fit(sigmoid, x, hist, p0=[-hist[0], 1/np.std(hist), np.mean(hist), 100])
            x_curve = np.linspace(min(x), max(x), 100)
            plt.plot(x_curve, sigmoid(x_curve, *params), 'r-', label='Sigmoid Fit')
            plt.text(0.65, 0.9, f'{round(params[0],2)}/[1+exp({round(params[1],2)} * (x-{round(params[2],2)}))] + {round(params[3],2)}', horizontalalignment='center', verticalalignment='center', transform=plt.gca().transAxes, fontsize=7)
            plt.show()
            if uniform:
                fig_expo.savefig(f'plots/exponential_uniform/{labels[label_num]}_ADC.png')
            else:
                fig_expo.savefig(f'plots/exponential_fits/{labels[label_num]}_ADC.png')
            plt.clf()
        
            
            
        '''
        #Fit Fermi Function
        alpha = .007297
        m = 1 #KeV NOT SURE ABOUT THIS VALUE
        c = 1   #m/s
        e = np.sqrt(4*np.pi*alpha) #C
        h = 1 # Joules-s / rad
        
        def S(Z):
            return np.sqrt(1-(alpha**2)*Z**2)
            
        def rho(): #change to variable if keeping constant
            return 3.4093e-15
        
        def E(T): #total energy
            return T + np.power(m,2)
        def P(T): #momentum
            return np.sqrt(np.power(E(T),2) - np.power(m,2))
         
        
        def eta(Z,T): #n
            return (Z*E(T)*np.power(e,2))/P(T)
        
        def F(Z,T):
            return 2*(1+S(Z))/((gamma(1+2*S(Z)))**2)*np.power(2*P(T)*rho(),2*S(Z)-2)*np.power(np.e,np.pi*eta(Z,T))*np.abs(gamma(S(Z)+eta(Z,T)*1j))**2 
        
        '''
        #simplified fermi function
        def F(Z,T): #fermi function
            numerator = (2*np.pi*eta(Z,T))
            denominator = (1-np.exp(-2*np.pi*eta(Z,T)))  
            return numerator/denominator   
            '''
        
        def N(T,C,Q,Z=background_max_energy[label_num][2]):
            return C*F(Z,T)*P(T)*E(T)*np.power((Q-T),2)
        
    if (label_num not in exlude_expo) and (len(background_max_energy[label_num]) > 2):
        scale = 100000
        start = 0 #shift
        cut = 0 #charge cut for fit

        # to fit the expression for beta decay onto the ADC graph, I had to first modify the histogram because the expression does not scale
        # Now just find endpoint, and reverse the modification to find on ADC graph 
        # i.e. Find endpoint of fit then multiply by scale to find ADC endpoint
        cut_charge_list_fit = [x/scale + start for x in charge_list if x/scale + start > cut ] # charge cut to cut out spike at low energies
        cut_charge_list_display = [x/scale + start for x in charge_list] #used to make histogram of all charges, cut is illustrated on graph with dotted line
        charge_list_recovered = [x/scale + start for x in charge_lists_recovered[label_num]]
        fig_fermi = plt.figure()
        hist_fit, bins, _ = plt.hist(cut_charge_list_fit,bins = 30)
        hist_fit_recovered, bins_r, _, = plt.hist(charge_list_recovered,bins = 30)

        #Not Recovered
        plt.clf()
        plt.hist(cut_charge_list_display,bins = 30)
        plt.title(f'Total Charge per {labels[label_num]} Cluster')  
        plt.xlabel(f'Total Charge per Cluster [{start} + ADC/10^5 ]')  
        plt.ylabel('Number of Clusters') 
        x = (bins[:-1] + bins[1:]) / 2  # Get x values for the center of each bin   
        x_r = (bins_r[:-1] + bins_r[1:]) / 2
        #Parameters vary widely
        #Initial parameter guessing 
        gueses = 1000
        initial_guesses_C = [random.randint(0, max(hist_fit)) for _ in range(gueses)]
        initial_guesses_Q = [random.uniform(0, 6) for _ in range(gueses)]

        best_r2 = -100
        best_C_param = 200
        best_Q_param = 1
        fit_fails = 0
        fit_success = 0
        best_chi = 0
        best_r2_r = -100
        best_C_param_r = 200
        best_Q_param_r = 1
        fit_fails_r = 0
        fit_success_r = 0
        best_chi_r = 0
        
        #Using r^2 value as a general method to find best parameters
        for i in range(gueses):
            try:
                params, pcov = curve_fit(N, x, hist_fit, p0=[initial_guesses_C[i],initial_guesses_Q[i]])
                fit_success+=1
                perr = np.sqrt(np.diag(pcov))
                y_pred = N(x, *params)
                R_squared = r2_score(hist_fit, y_pred)
                #Chi = chisquare(hist_fit, y_pred)
                if R_squared > best_r2:
                    best_r2 = R_squared
                    best_C_param = params[0]
                    best_Q_param = params[1]
                    best_perr = perr
            except:
                if fit_fails>100:
                    break
                fit_fails+=1
    
            try:
                params_recovered, pcov_recovered = curve_fit(N, x_r, hist_fit_recovered, p0=[initial_guesses_C[i],initial_guesses_Q[i]])
                fit_success_r+=1
                perr_r = np.sqrt(np.diag(pcov_recovered))
                y_pred_r = N(x_r, *params_recovered)
                R_squared_r = r2_score(hist_fit_recovered, y_pred_r)
                #Chi = chisquare(hist_fit, y_pred)
                if R_squared_r > best_r2_r:
                    best_r2_r = R_squared_r
                    best_C_param_r = params_recovered[0]
                    best_Q_param_r = params_recovered[1]
                    best_perr_r = perr_r
            except:
                if fit_fails_r>100:
                    break
                fit_fails_r+=1
            
        #find endpoint using fermi fit ^
        print(f'Fit sucess for {labels[label_num]}: {fit_success}, Fails: {fit_fails}')
        print(f'Recovered Fit sucess for {labels[label_num]}: {fit_success_r}, Fails: {fit_fails_r}')

        
        x = (bins[:-1] + bins[1:]) / 2  # Get x values for the center of each bin
        x_curve = np.linspace(min(x), max(x)+ max(x)/6, 100)

        #create parameter samples within their error
        '''
        sample_size = 20
        sampled_C_params = []
        sampled_Q_params = []

        for i in range(sample_size):
            sample_C = np.random.normal(best_C_param, best_perr[0])
            sample_Q = np.random.normal(best_Q_param, best_perr[1])
            sampled_C_params.append(sample_C)
            sampled_Q_params.append(sample_Q)
        
        #unc_count = 0
        endpoints = []
        #use random sample parameters and find minimum on each different curve
        for parameter_C, parameter_Q in zip(sampled_C_params,sampled_Q_params):
            fit_min = 100
            T_max = 0
            y = N(x_curve,parameter_C,parameter_Q)
            min_x = x_curve[y.argmin()]
            
            
            for i in range(int((end-start)/step)+1):
                energy_adc = start + i*step
                val = N(energy_adc,parameter)
                if val < 40:
                    #print("val < 3")
                    #unc_count+=step #uncertainty measured by how "steep" the curve minimum is
                    if val < fit_min: #find minimum of curve
                        fit_min = val
                        T_max = energy_adc
                        
                
            endpoints.append(min_x)
            '''
        
                #print(f"Endpoint for {labels[label_num]} is {T}")
        max_endpoint = (best_Q_param-start)*scale
        max_endpoint_r = (best_Q_param_r-start)*scale

        #uncertainty = np.std(endpoints*scale)
        print(f'Adding endpoint {max_endpoint} to {labels[label_num]}')
        max_charge_fermi[label_num] = max_endpoint #record energy at minimum value of curve 
        max_charge_fermi_r[label_num] = max_endpoint_r
        print(f'labelnum: {label_num} with {background_max_energy[label_num]}')
        if fit_success>=1:
            background_max_energy[label_num][3] = best_perr[1] * scale # uncertainty as uncertainty in Q value
            
        else:
            background_max_energy[label_num][3] = 0
            
        
        #DEBUGGING nan values returned from N at values below x=.25
        '''
        for x in x_curve:
            print(f'x: {x}')
            print(f'N: {N(x,best_C_param,best_Q_param)}')
            print(f'Inside P(T) square root:  {np.power(E(x),2) - np.power(m,2)} ')
            '''
        #manuelly plug in conversion to display on graph. beta slope obtained only after running script
        #NEED TO CHANGE IF BACKGROUNDS CHANGE
        beta_slope = 9.61268413759387e-06
        def conversion(ADC):
            return beta_slope*ADC+.29
        
        x = (bins[:-1] + bins[1:]) / 2  # Get x values for the center of each bin
        x_curve = np.linspace(min(x), best_Q_param, 100)

        plt.plot(x_curve, N(x_curve,best_C_param,best_Q_param), 'r-', label='Beta Fit') #fit with Q and Z not constants, varied for fit
        
        plt.axvline(x=best_Q_param, linestyle='--', color='purple', label='Fitted Q value')
        plt.axvline(x=cut, linestyle='--', color='black', label='Cut')

        plt.text(1.05, 0.75, f'Label Number: {label_num}\nR-squared: {best_r2:.2f}\nFitted Q Value: {best_Q_param:.3f}\nADC: {max_endpoint:.0f} ADC\nActual: {background_max_energy[label_num][1]} MeV ', transform=plt.gca().transAxes, fontsize=8, verticalalignment='top')
        plt.legend(loc='upper left', bbox_to_anchor=(1.02, 1),fontsize=8)
        plt.subplots_adjust(right=0.74)
        print(f'Params for {labels[label_num]}: {best_C_param},{best_Q_param}')
        if uniform:
            fig_fermi.savefig(f'plots/with_fermi_uniform/{labels[label_num]}_ADC.png')
        else:
            fig_fermi.savefig(f'plots/with_fermi/{labels[label_num]}_ADC.png')

        plt.clf()
        
        #recovered
        fig_fermi_recovered = plt.figure()
        plt.hist(charge_list_recovered,bins = 30)
        plt.title(f'Total Charge per {labels[label_num]} Cluster REcovered')  
        plt.xlabel(f'Total Charge per Cluster [{start} + ADC/10^5 ]')  
        plt.ylabel('Number of Clusters') 
        x_r = (bins_r[:-1] + bins_r[1:]) / 2  # Get x values for the center of each bin
        x_curve_r = np.linspace(min(x_r), best_Q_param_r, 100)
        plt.plot(x_curve_r, N(x_curve_r,best_C_param_r,best_Q_param_r), 'r-', label='Beta Fit') #fit with Q and Z not constants, varied for fit
        
        plt.axvline(x=best_Q_param_r, linestyle='--', color='purple', label='Fitted Q value')
        plt.axvline(x=cut, linestyle='--', color='black', label='Cut')

        plt.text(1.05, 0.75, f'Label Number: {label_num}\nR-squared: {best_r2_r:.2f}\nFitted Q Value: {best_Q_param_r:.3f}\nADC: {max_endpoint_r:.0f} ADC\nActual: {background_max_energy[label_num][1]} MeV ', transform=plt.gca().transAxes, fontsize=8, verticalalignment='top')
        plt.legend(loc='upper left', bbox_to_anchor=(1.02, 1),fontsize=8)
        plt.subplots_adjust(right=0.74)
        print(f'Params for recovered {labels[label_num]}: {best_C_param_r},{best_Q_param_r}')
        if uniform:
            fig_fermi_recovered.savefig(f'plots/with_fermi_uniform_recovered/{labels[label_num]}_ADC.png')
        else:
            print("No save location for non uniform")
            #fig_fermi.savefig(f'plots/with_fermi/{labels[label_num]}_ADC.png')

        plt.clf()
        plt.close() 
        #plot parameters vs endpoints
        fig_params = plt.figure()
        #plt.scatter(sampled_Q_params,endpoints,color = 'blue', label='Q params')
        #plt.scatter(sampled_C_params,endpoints,color = 'red', label='C params')
        plt.ylabel("Calculated Endpoint")
        plt.xlabel("Fit parameter")

        plt.title(f"Endpoint Dependency on Fit Parameter {labels[label_num]}")
        plt.legend()
        if uniform:
            fig_params.savefig(f'plots/with_fermi_uniform/parameters/parameter_vs_endpoints_{labels[label_num]}.png')
        else:
            print("bla")
        plt.close() 
            

    
 
#plot all background charges on same hist   
fig4 = plt.figure()
plt.hist(total_charges, bins = 100)
plt.ylabel("Number of Clusters")
plt.xlabel("Total Charge per Cluster [ADC]")
plt.yscale("log")
plt.title("Cluster Charge for all Events")
if uniform:
    fig4.savefig('plots/backgrounds_uniform/total_charge.png')
else:
    fig4.savefig('plots/backgrounds/total_charge.png')
plt.close() 



#create two lists for beta and alpha decay background endpoints for linear fit
beta_adc_fermi = []
beta_adc_fermi.append(0)
alpha_adc_fermi = []
uncertainties = []
uncertainties.append(0)

for key in background_max_energy:
    print("Background: " + labels[key] + " Max ADC using Fermi: " + str(max_charge_fermi[key]))
    '''
    if key == 2:
        beta_adc_fermi.append(41000)
        beta_MeV.append(background_max_energy[2][1])
        beta_adc.append(max_charge[key])
        uncertainties.append(10000)
        '''
    if key not in exlude:
        if background_max_energy[key][0] == 0:
            beta_adc.append(max_charge[key])
            beta_adc_fermi.append(max_charge_fermi[key])
            beta_MeV.append(background_max_energy[key][1])
            uncertainties.append(background_max_energy[key][3])
            
        elif background_max_energy[key][0] == 1:
            alpha_adc.append(max_charge[key])
            alpha_adc_fermi.append(max_charge_fermi[key])
            alpha_MeV.append(background_max_energy[key][1])
            
#manually add alpha values
alpha_adc_fermi[0] = 8000
alpha_MeV[0] = background_max_energy[6][1]
alpha_adc_fermi[1] = 10700
alpha_MeV[1] = background_max_energy[7][1]
print(f'Alpha ADC Fermi: {alpha_adc_fermi}')


#Linear FIT CONVERSION USING FERMI ENDPOINTS
slope_B, intercept_B, r_value_B, p_value_B, std_err_B = linregress(beta_adc_fermi, beta_MeV)
slope_a, intercept_a, r_value_a, p_value_a, std_err_a = linregress(alpha_adc_fermi, alpha_MeV)
fig_conv = plt.figure()
beta_fit_line = f'Beta Fit: MeV = {slope_B:.2e} * ADC + {intercept_B:.2f} (R²={r_value_B**2:.2f})'
#alpha_fit_line = f'Alpha Fit: MeV = {slope_a:.2e} * ADC + {intercept_a:.2f} (R²={r_value_a**2:.2f})'
print("Beta slope: " + str(slope_B))
print("Alpha slope: " + str(slope_a))
#plt.scatter(beta_adc_fermi, beta_MeV, xerr=uncertainties, color = 'red', label='Beta Decay Backgrounds')
#plt.scatter(alpha_adc_fermi,alpha_MeV, color = 'blue', label='Alpha Decay Backgrounds')
plt.errorbar(beta_adc_fermi, beta_MeV, xerr=uncertainties, fmt='o', color='red', label='Beta Decay Backgrounds')
plt.plot(beta_adc_fermi, [slope_B * x + intercept_B for x in beta_adc_fermi], color='red', label=beta_fit_line)
#plt.plot(alpha_adc_fermi,[slope_a * x + intercept_a for x in alpha_adc_fermi], color='blue', label=alpha_fit_line)

exclude_str = ', '.join(str(labels[val]) for val in exlude)

# Create the plot title using the formatted string
plt.title(f'ADC to MeV using Fermi endpoints')
plt.xlabel('ADC')
plt.ylabel('MeV')
plt.legend()
if uniform:
    fig_conv.savefig('plots/ADC_KeV_uniform/beta_fit_FERMI.png')
else:
    fig_conv.savefig('plots/ADC_KeV_fits/beta_fit_FERMI.png')
plt.clf()
plt.close() 

'''
#linear fit
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
if uniform:
    fig0.savefig('plots/ADC_KeV_uniform/beta_fit_raw_endpoints.png')
else:
    fig0.savefig('plots/ADC_KeV_fits/beta_fit_raw_endpoints.png')


plt.clf()
plt.close() 
'''
#copy plot to local: scp hakins@lxplus.cern.ch:/afs/cern.ch/user/h/hakins/tpgtools/scripts/plots/histogram.png C:\Users\harry\Documents\Test_Plots