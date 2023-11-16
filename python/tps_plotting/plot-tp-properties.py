# look at the notebook and do the same here
import numpy as np
import matplotlib.pyplot as plt
import argparse

from include.TriggerPrimitive import TriggerPrimitive as TriggerPrimitive
from include.PlottingUtils import *

# parse from command line the args.files and the number of tps to plot
parser = argparse.ArgumentParser()
parser.add_argument("-f", "--files", nargs="+", help="files to read")
parser.add_argument("-e", "--output-folder", type=str, help="output folder", default="./output/")
parser.add_argument("-n", "--number-tps", type=int, help="number of tps to plot")
parser.add_argument("-s", "--superimpose", action="store_true", help="superimpose plots")
parser.add_argument("-a", "--all", action="store_true", help="plot all the TP variables")
parser.add_argument("-t", "--time-peak", action="store_true", help="plot time peak")
parser.add_argument("-o", "--time-over-threshold", action="store_true", help="plot time over threshold")
parser.add_argument("-i", "--adc-integral", action="store_true", help="plot adc integral")
parser.add_argument("-p", "--adc-peak", action="store_true", help="plot adc peak")
parser.add_argument("-c", "--channel", action="store_true", help="plot channel")
parser.add_argument("-d", "--detid", action="store_true", help="plot detid")
args = parser.parse_args()

if args.all:
    args.time_peak = True
    args.time_over_threshold = True
    args.adc_integral = True
    args.adc_peak = True
    args.channel = True
    args.detid = True
    
# read the file(s) and create arrays of TPs, using the class in TriggerPrimitive.py
# the code will deduce if the file is coming from offline or online data

tps_lists = []
for tpFile_path in  args.files:
    
    print ("Reading file: " + tpFile_path)
    
    this_tps_list = saveTPs(tpFile_path, args.number_tps)
    tps_lists.append(this_tps_list)



if args.time_peak:
    plotTimePeak(tps_lists, args.files, 
                 superimpose=args.superimpose, 
                 quantile=1, 
                 y_min=None, y_max=None,
                 output_folder=args.output_folder,
                 show=False) 

if args.time_over_threshold:
    plotTimeOverThreshold(tps_lists, args.files,
                            superimpose=args.superimpose,
                            quantile=1,
                            y_min=None, y_max=None,
                            output_folder=args.output_folder,
                            show=False)

if args.adc_integral:
    plotADCIntegral(tps_lists, args.files,
                    superimpose=args.superimpose,
                    quantile=1,
                    y_min=None, y_max=None,
                    output_folder=args.output_folder,
                    show=False)

if args.adc_peak:
    plotADCPeak(tps_lists, args.files,
                superimpose=args.superimpose,
                quantile=1,
                y_min=None, y_max=None,
                output_folder=args.output_folder,
                show=False)

if args.channel:
    plotChannel(tps_lists, args.files,
                superimpose=args.superimpose,
                x_min=None, x_max=None,
                y_min=None, y_max=None,
                output_folder=args.output_folder,
                show=False)
            

if args.detid:
    plotDetId(tps_lists, args.files,
                superimpose=args.superimpose,
                quantile=1,
                y_min=None, y_max=None,
                output_folder=args.output_folder,
                show=False)
