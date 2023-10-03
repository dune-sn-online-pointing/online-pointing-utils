import numpy as np
import sys
import matplotlib.pyplot as plt
import time
import os
import argparse
import warnings
import gc

import create_images_from_tps_libs as tp2img


# write a parser to set up the running from command line
parser = argparse.ArgumentParser(description='Tranforms Trigger Primitives to images.')
parser.add_argument('--input_file', type=str, help='Input file name')
parser.add_argument('--chanmap', type=str, default='FDHDChannelMap_v1_wireends.txt', help='path to the file with Channel Map')
parser.add_argument('--show', action='store_true', help='show the image')
parser.add_argument('--save_img', action='store_true', help='save the image')
parser.add_argument('--save_ds', action='store_true', help='save the dataset')
parser.add_argument('--write', action='store_true', help='write the clusters to a file')
parser.add_argument('--output_path', type=str, default='/eos/user/d/dapullia/tp_dataset/', help='path to save the image')
parser.add_argument('--img_save_folder', type=str, default='images/', help='folder to save the image')
parser.add_argument('--img_save_name', type=str, default='image', help='name to save the image')
parser.add_argument('--n_events', type=int, default=0, help='number of events to process')
parser.add_argument('--min_tps_to_cluster', type=int, default=4, help='minimum number of TPs to create a cluster')

args = parser.parse_args()
# unpack the arguments
filename = args.input_file

chanMap = args.chanmap
show = args.show
save_img = args.save_img
save_ds = args.save_ds
write = args.write
output_path = args.output_path
n_events = args.n_events
min_tps_to_cluster = args.min_tps_to_cluster
img_save_folder = args.img_save_folder
img_save_name = args.img_save_name
save_path = output_path


'''
TP format (snana_hits):  
[0] time_start
[1] time_over_threshold
[2] time_peak
[3] channel
[4] adc_integral
[5] adc_peak
[6] detID
[7] type

TP format (tpstream):  
[0] time_start
[1] time_over_threshold
[2] time_peak
[3] channel
[4] adc_integral
[5] adc_peak
[6] detID
[7] flag
[8] version

'''


'''
Channel Map format (apa):
[0] offlchan    in gdml and channel sorting convention
[1] upright     0 for inverted, 1 for upright
[2] wib         1, 2, 3, 4 or 5  (slot number +1?)
[3] link        link identifier: 0 or 1
[4] femb_on_link    which of two FEMBs in the WIB frame this FEMB is:  0 or 1
[5] cebchan     cold electronics channel on FEMB:  0 to 127
[6] plane       0: U,  1: V,  2: X
[7] chan_in_plane   which channel this is in the plane in the FEMB:  0:39 for U and V, 0:47 for X
[8] femb       which FEMB on an APA -- 1 to 20
[9] asic       ASIC:   1 to 8
[10] asicchan   ASIC channel:  0 to 15
[11] wibframechan   channel index in WIB frame (used with get_adc in detdataformats/WIB2Frame.hh).  0:255
'''



if __name__=='__main__':

    #read first n_events lines in the data from txt, all are ints
    if n_events:
        all_TPs = np.loadtxt(filename, skiprows=0, max_rows=n_events, dtype=int)
    else:
        all_TPs = np.loadtxt(filename, skiprows=0, dtype=int)
    
    #read channel map
    channel_map = tp2img.create_channel_map_array(chanMap)
    # The line below is needed because the channel map is not the same (pain)
    channel_map[:,6] = channel_map[:,7]


    #create images
    # clusters = tp2img.cluster_maker(all_TPs, channel_map, ticks_limit=550, channel_limit=20, min_tps_to_cluster=min_tps_to_cluster)
    clusters = tp2img.cluster_maker_only_by_time(all_TPs, channel_map, ticks_limit=400, channel_limit=20, min_tps_to_cluster=min_tps_to_cluster)
    print('Number of clusters: ', len(clusters))
    for i, cluster in enumerate(clusters):
        if show:
            tp2img.show_img(np.array(cluster), channel_map, min_tps_to_create_img=2, make_fixed_size=True, width=70, height=1000, x_margin=5, y_margin=50)
        if save_img:
            if not os.path.exists(output_path+img_save_folder):
                os.makedirs(output_path+img_save_folder)
            tp2img.save_img(np.array(cluster), channel_map, output_path=output_path+img_save_folder, outname=img_save_name+str(i), min_tps_to_create_img=2, make_fixed_size=True, width=120, height=1500, x_margin=5, y_margin=400)
    # write the clusters to a file
    if write:
        with open('clusters.txt', 'w') as f:
            for i, cluster in enumerate(clusters):
                f.write('Cluster'+str(i)+':\n')
                for tp in cluster:
                    f.write(str(tp[0]) + ' ' + str(tp[1]) + ' ' + str(tp[2]) + ' ' + str(tp[3]) + ' ' + str(tp[4]) + ' ' + str(tp[5]) + ' ' + str(tp[6]) + ' ' + str(tp[7]) + '\n')
                f.write('\n')
        
    # do some statistics on the clusters

    n_spurious_clusters = 0
    hist_types = np.array([0,0,0,0,0,0,0,0,0,0])
    for cluster in clusters:
        temp = cluster[0][7]
        for tp in cluster:
            if tp[7] != temp:
                n_spurious_clusters += 1
                break
        else:
            hist_types[cluster[0][7]] += 1
    print('Number of clusters with different type: ', n_spurious_clusters)

    print('Types: ', hist_types)


    if save_ds:
        if not os.path.exists(save_path):
            os.makedirs(save_path)
        # create the dataset
        print('--------- Python list version with numpy array and preallocated array --------')
        dataset_img, dataset_label = tp2img.create_dataset(clusters, channel_map=channel_map, make_fixed_size=True, width=70, height=1000, x_margin=5, y_margin=50)


        print('Dataset shape: ', (dataset_img).shape)
        print('Labels shape: ', (dataset_label).shape)            
        np.save(save_path + 'dataset_img.npy', dataset_img)
        np.save(save_path + 'dataset_lab.npy', dataset_label)
        
        print(np.unique(dataset_label,return_counts=True))
       

    print("Done!")

