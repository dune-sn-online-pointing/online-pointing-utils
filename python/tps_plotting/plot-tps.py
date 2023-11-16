# look at the notebook and do the same here
import numpy as np
import matplotlib.pyplot as plt
import argparse

from include.TriggerPrimitive import TriggerPrimitive as TriggerPrimitive
from include.PlottingUtils import *

# parse from command line the args.files and the number of tps to plot
parser = argparse.ArgumentParser()
parser.add_argument("-f", "--files", nargs="+", help="files to read")
parser.add_argument("-n", "--number-tps", type=int, help="number of tps to plot")
parser.add_argument("-s", "--superimpose", action="store_true", help="superimpose plots")
args = parser.parse_args()

# read the file(s) and create arrays of TPs, using the class in TriggerPrimitive.py
# the code will deduce if the file is coming from offline or online data

tps_lists = []
for tpFile_path in  args.files:
    
    print ("Reading file: " + tpFile_path)
    
    this_tps_list = saveTPs(tpFile_path, args.number_tps)
    # order them basing on start_time
    this_tps_list.sort(key=lambda x: x.time_start)
    tps_lists.append(this_tps_list)


plotTimePeak(tps_lists, args.files, 
             superimpose=args.superimpose, 
             quantile=1, 
             y_min=None, y_max=None,
             show=False) 

plotTimeOverThreshold(tps_lists, args.files,
                        superimpose=args.superimpose,
                        quantile=1,
                        y_min=None, y_max=None,
                        show=False)

plotADCIntegral(tps_lists, args.files,
                superimpose=args.superimpose,
                quantile=1,
                y_min=None, y_max=None,
                show=False)

plotADCPeak(tps_lists, args.files,
            superimpose=args.superimpose,
            quantile=1,
            y_min=None, y_max=None,
            show=False)

plotChannel(tps_lists, args.files,
            superimpose=args.superimpose,
            x_min=None, x_max=None,
            y_min=None, y_max=None,
            show=False)
            

plotDetId(tps_lists, args.files,
            superimpose=args.superimpose,
            quantile=1,
            y_min=None, y_max=None,
            show=False)