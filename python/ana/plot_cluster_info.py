import numpy as np
import matplotlib.pyplot as plt
import os
import sys
import argparse

# need pyroot
import ROOT



# main
if __name__ == "__main__":
    
    parser = argparse.ArgumentParser(description="Plot cluster info from a ROOT file.")
    parser.add_argument("-i", "--input",             required=True,                 help="Path to input ROOT file.")
    parser.add_argument("-o", "--output",            type=str, default="./",        help="Output folder (default: ./)")
    
    print ("Starting plot_cluster_info.py")
    
    # open file
    args = parser.parse_args()
    input_file = args.input
    output_folder = args.output
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
    if not os.path.exists(input_file):
        print(f"Input file {input_file} does not exist.")
        sys.exit(1)
    # open the file
    input_file = ROOT.TFile(input_file)
    # get the tree
    tree = input_file.Get("clusters_tree_X") # TODO all views
    # get the number of entries
    nclusters = tree.GetEntries()
    print(f"Number of entries: {nclusters}")
    # get the branches, one is supernova_tp_fraction
    
    # # plot the supernova fraction
    # supernova_tp_fraction = []
    # for entry in tree:
    #     supernova_tp_fraction.append(entry.supernova_tp_fraction)

    # plt.hist(supernova_tp_fraction, bins=50, color='blue', alpha=0.7)
    # plt.xlabel("Supernova TP Fraction")
    # plt.ylabel("Frequency")
    # plt.title("Histogram of Supernova TP Fraction")
    # plt.grid(True)
    # plt.savefig(os.path.join(output_folder, "supernova_tp_fraction_histogram.png"))
    # plt.close()
    # print (f"Saved histogram of supernova TP fraction to {os.path.join(output_folder, 'supernova_tp_fraction_histogram.png')}")
    
    
    # # plot the particle true energy
    # true_particle_energy = []
    # for entry in tree:
    #     true_particle_energy.append(entry.true_particle_energy)
    # plt.hist(true_particle_energy, bins=50, color='blue', alpha=0.7)
    # plt.xlabel("True Particle Energy")
    # plt.ylabel("Frequency")
    # plt.title("Histogram of True Particle Energy")
    # plt.grid(True)
    # plt.savefig(os.path.join(output_folder, "true_particle_energy_histogram.png"))
    # plt.close()
    # print (f"Saved histogram of true particle energy to {os.path.join(output_folder, 'true_particle_energy_histogram.png')}")
    
    # # plot the neutrino true energy
    # true_neutrino_energy = []
    # for entry in tree:
    #     true_neutrino_energy.append(entry.true_neutrino_energy)
    # plt.hist(true_neutrino_energy, bins=50, color='blue', alpha=0.7)
    # plt.xlabel("True Neutrino Energy")
    # plt.ylabel("Frequency")
    # plt.title("Histogram of True Neutrino Energy")
    # plt.grid(True)
    # plt.savefig(os.path.join(output_folder, "true_neutrino_energy_histogram.png"))
    # plt.close()
    # print (f"Saved histogram of true neutrino energy to {os.path.join(output_folder, 'true_neutrino_energy_histogram.png')}")
    
    # plot adc_peak, each entry contains a vector of values. plot all values
    
    print ("Plotting ADC peak")
    adc_peak = []
    for entry in tree:
        val = entry.tp_adc_peak
        if hasattr(val, '__iter__') and not isinstance(val, (str, bytes)):
            adc_peak.extend(val)
        else:
            adc_peak.append(val)
    
    # plot all values on the same plot
    plt.hist(adc_peak, bins=50, color='blue', alpha=0.7)
    plt.xlabel("ADC Peak")
    plt.ylabel("Frequency")
    plt.title("Histogram of ADC Peak")
    plt.grid(True)
    plt.savefig(os.path.join(output_folder, "adc_peak_histogram.png"))
    plt.close()
    print (f"Saved histogram of ADC peak to {os.path.join(output_folder, 'adc_peak_histogram.png')}")