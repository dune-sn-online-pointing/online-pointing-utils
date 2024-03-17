"""
 * @file cluster_maker.py
 * @brief Reads takes tps array and makes clusters.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2024.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
"""

import numpy as np

# This function creates a channel map array, 
# where the first column is the channel number and the second column 
# is the plane view (0 for U, 1 for V, 2 for X).
# TODO understand if this can be avoided by reading from detchannelmaps
def create_channel_map_array(which_detector="APA"):
    '''
    :param which_detector, options are: 
     - APA for Horizontal Drift 
     - CRP for Vertical Drift (valid also for coldbox)
     - 50L for 50L detector, VD technology
    :return: channel map array
    '''
    # create the channel map array
    if which_detector == "APA":
        channel_map = np.empty((2560, 2), dtype=int)
        channel_map[:, 0] = np.linspace(0, 2559, 2560, dtype=int)
        channel_map[:, 1] = np.concatenate((np.zeros(800), np.ones(800), np.ones(960)*2))        
    elif which_detector == "CRP":
        channel_map = np.empty((3072, 2), dtype=int)
        channel_map[:, 0] = np.linspace(0, 3071, 3072, dtype=int)
        channel_map[:, 1] = np.concatenate((np.zeros(952), np.ones(952), np.ones(1168)*2))
    elif which_detector == "50L":
        channel_map = np.empty((128, 2), dtype=int)
        channel_map[:, 0] = np.linspace(0, 127, 128, dtype=int)
        # channel_map[:, 1] = np.concatenate(np.ones(49)*2, np.zeros(39), np.ones(40))
        channel_map[:, 1] = np.concatenate((np.zeros(39), np.ones(40), np.ones(49)*2))
    return channel_map

def distance_between_clusters(cluster1, cluster2):
    '''
    :param cluster1: first cluster
    :param cluster2: second cluster
    :return: distance between the two clusters
    '''
    pos1 = cluster1.get_reco_pos()
    pos2 = cluster2.get_reco_pos()
    return np.linalg.norm(pos1 - pos2)


