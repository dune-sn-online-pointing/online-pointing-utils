import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import ImageGrid
import time
import os
import argparse
import warnings
import sys
import gc


'''
TP format (snana_hits.txt)):  
[0] time_start
[1] time_over_threshold
[2] time_peak
[3] channel
[4] adc_integral
[5] adc_peak
[6] detID
[7] type
'''


'''
Channel Map format:
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


def create_channel_map_array(chanMap):
    '''
    :param chanMap: path to the channel map 
    :return: channel map array
    '''
    channel_map = np.loadtxt(chanMap, skiprows=0, dtype=int)
    # sort the channel map by the first column (offlchan)
    channel_map = channel_map[np.argsort(channel_map[:, 0])]
    return channel_map


def from_tp_to_imgs(tps, make_fixed_size=False, width=500, height=1000, x_margin=10, y_margin=100, y_min_overall=-1, y_max_overall=-1):
    '''
    :param tps: all trigger primitives to draw
    :param make_fixed_size: if True, the image will have fixed size, otherwise it will be as big as the TPs
    :param width: width of the image
    :param height: height of the image
    :param x_margin: margin on the x axis
    :param y_margin: margin on the y axis
    :return: image
    '''

    if y_min_overall != -1:
        t_start = y_min_overall
    else:
        t_start = tps[0, 0] 

    if y_max_overall != -1:
        t_end = y_max_overall
    else:
        t_end = np.max(tps[:, 0] + tps[:, 1])
    
    x_max = (tps[:, 3].max())
    x_min = (tps[:, 3].min())

    x_range = x_max - x_min 
    y_range = int((t_end - t_start))

    # create the image
    if make_fixed_size:
        img_width = width
        img_height = height
    else:
        img_width =  x_range + 2*x_margin
        img_height =  y_range + 2*y_margin
    img = np.zeros((img_height, img_width))
    # fill image
    if (not make_fixed_size):
        for tp in tps:
            x = (tp[3] - x_min) + x_margin
            y_start = (tp[0] - t_start) + y_margin
            y_end = (tp[0] + tp[1] - t_start) + y_margin
            img[int(y_start)-1:int(y_end)-1, int(x)-1] = tp[4]/(y_end - y_start)


    else:
    # We stretch the image inwards if needed but we do not upscale it. In this second case we build a padded image
        if img_width < x_range:
            stretch_x = True
            print('Warning: image width is smaller than the range of the TPs. The image will be stretched inwards.')

        else:
            x_margin = (img_width - x_range)/2
            stretch_x = False
        
        if img_height < y_range:
            stretch_y = True
            print('Warning: image height is smaller than the range of the TPs. The image will be stretched inwards.')

        else:
            y_margin = (img_height - y_range)/2
            stretch_y = False


        if stretch_x & stretch_y:
            for tp in tps:
                x=(tp[3] - x_min)/x_range * (img_width - 2*x_margin) + x_margin
                y_start = (tp[0] - t_start)/y_range * (img_height - 2*y_margin) + y_margin
                y_end = (tp[0] + tp[1] - t_start)/y_range * (img_height - 2*y_margin) + y_margin
                img[int(y_start)-1:int(y_end)-1, int(x)-1] = tp[4]/(y_end - y_start)
        elif stretch_x:
            for tp in tps:                
                x=(tp[3] - x_min)/x_range * (img_width - 2*x_margin) + x_margin
                y_start = (tp[0] - t_start) + y_margin
                y_end = (tp[0] + tp[1] - t_start) + y_margin
                img[int(y_start)-1:int(y_end)-1, int(x)-1] = tp[4]/(y_end - y_start)
        elif stretch_y:
            for tp in tps:
                x = (tp[3] - x_min) + x_margin
                y_start = (tp[0] - t_start)/y_range * (img_height - 2*y_margin) + y_margin
                y_end = (tp[0] + tp[1] - t_start)/y_range * (img_height - 2*y_margin) + y_margin
                img[int(y_start):int(y_end)-1, int(x)-1] = tp[4]/(y_end - y_start)

        else:
            for tp in tps:
                x = (tp[3] - x_min) + x_margin
                y_start = (tp[0] - t_start) + y_margin
                y_end = (tp[0] + tp[1] - t_start) + y_margin
                img[int(y_start)-1:int(y_end)-1, int(x)-1] = tp[4]/(y_end - y_start)
   
    return img




def all_views_img_maker(tps, channel_map, min_tps_to_create_img=2, make_fixed_size=False, width=500, height=1000, x_margin=10, y_margin=200):
    '''
    :param tps: all trigger primitives in the event
    :param channel_map: channel map
    :param min_tps: minimum number of TPs to create the image
    :param make_fixed_size: if True, the image will have fixed size, otherwise it will be as big as the TPs
    :param width: width of the image if make_fixed_size is True
    :param height: height of the image if make_fixed_size is True
    :param x_margin: margin on the x axis if make_fixed_size is False
    :param y_margin: margin on the y axis if make_fixed_size is False
    :return: images or -1 if there are not enough TPs
    '''
    # U plane, take only the tps where the corrisponding position in the channel map is 0      
    tps_u = tps[np.where(channel_map[tps[:, 3], 6] == 0)]

    # V plane, take only the tps where the corrisponding position in the channel map is 1
    tps_v = tps[np.where(channel_map[tps[:, 3], 6] == 1)]

    # X plane, take only the tps where the corrisponding position in the channel map is 2
    tps_x = tps[np.where(channel_map[tps[:, 3], 6] == 2)]

    img_u, img_v, img_x = np.array([[-1]]), np.array([[-1]]), np.array([[-1]])


    y_min_overall = tps[0,0]
    y_max_overall = np.max(tps[:,0] + tps[:,1])

    if tps_u.shape[0] >= min_tps_to_create_img:
        img_u = from_tp_to_imgs(tps_u, make_fixed_size=make_fixed_size, width=width, height=height, x_margin=x_margin, y_margin=y_margin, y_min_overall=y_min_overall, y_max_overall=y_max_overall)

    if tps_v.shape[0] >= min_tps_to_create_img: 
        img_v = from_tp_to_imgs(tps_v, make_fixed_size=make_fixed_size, width=width, height=height, x_margin=x_margin, y_margin=y_margin, y_min_overall=y_min_overall, y_max_overall=y_max_overall)

    if tps_x.shape[0] >= min_tps_to_create_img:
        img_x = from_tp_to_imgs(tps_x, make_fixed_size=make_fixed_size, width=width, height=height, x_margin=x_margin, y_margin=y_margin, y_min_overall=y_min_overall, y_max_overall=y_max_overall)

    return img_u, img_v, img_x




def cluster_maker(all_tps, channel_map, ticks_limit=100, channel_limit=20, min_tps_to_cluster=4):
    '''
    :param all_tps: all trigger primitives in the event
    :param channel_map: channel map
    :param ticks_limit: maximum time window to consider
    :param channel_limit: maximum number of channels to consider
    :param min_hits: minimum number of hits to consider
    :return: list of clusters
    '''

    # create a list of clusters
    clusters = []
    buffer = []
    # loop over the TPs
    for tp in all_tps:
        tp = [tp[0], tp[1], tp[2], tp[3], tp[4], tp[5], tp[6], tp[7]]
        tp_type = channel_map[tp[3], 6]
        if len(buffer) == 0:
            buffer.append([tp])
            
        else:
            appended = False
            buffer_copy = buffer.copy()
            buffer = []
            for i, elem in enumerate(buffer_copy):
                there_is_u = False
                there_is_v = False
                there_is_x = False
                if (tp[0] - elem[-1][0]-elem[-1][1]) < ticks_limit:
                # if abs(tp[2] - elem[-1][2]) < ticks_limit:
                    for tp_i in elem:
                        if channel_map[tp_i[3], 6] ==0:
                            there_is_u = True
                        elif channel_map[tp_i[3], 6] ==1:
                            there_is_v = True
                        elif channel_map[tp_i[3], 6] ==2:
                            there_is_x = True

                        if (tp_type == tp_i[3]):
                            if abs(tp[3] - tp_i[3]) < channel_limit:                         
                                elem.append(tp)
                                appended = True
                            buffer.append(elem)
                            break
                    else:
                        if (tp_type == 0) and (not there_is_u):
                            elem.append(tp)
                            appended = True
                            buffer.append(elem)
                        elif (tp_type == 1) and (not there_is_v):
                            elem.append(tp)
                            appended = True
                            buffer.append(elem)
                        elif (tp_type == 2) and (not there_is_x):
                            elem.append(tp)
                            appended = True
                            buffer.append(elem)
                        else:
                            buffer.append(elem)
                elif len(elem) >= min_tps_to_cluster:
                    clusters.append(elem)
            if not appended:
                buffer.append([tp])

    return clusters

def cluster_maker_only_by_time(all_tps, channel_map, ticks_limit=100, channel_limit=20, min_tps_to_cluster=4):
    clusters = []
    current_cluster = np.array([[all_tps[0][0], all_tps[0][1], all_tps[0][2], all_tps[0][3], all_tps[0][4], all_tps[0][5], all_tps[0][6], all_tps[0][7]]])
    for tp in all_tps[1:]:
        tp = [tp[0], tp[1], tp[2], tp[3], tp[4], tp[5], tp[6], tp[7]]
        if (tp[0] - np.max(current_cluster[:, 0]+current_cluster[:,1])) < ticks_limit:
            current_cluster = np.append(current_cluster, [tp], axis=0)
        else:
            if len(current_cluster) >= min_tps_to_cluster:
                clusters.append(current_cluster)
            current_cluster = np.array([tp])
            

    return clusters







def show_img(all_TPs, channel_map, min_tps_to_create_img=2, make_fixed_size=False, width=500, height=1000, x_margin=10, y_margin=200):
    '''
    :param all_TPs: all trigger primitives in the event
    :param channel_map: channel map
    :param outname: base name for the images
    :param min_tps: minimum number of TPs to create the image
    :param make_fixed_size: if True, the image will have fixed size, otherwise it will be as big as the TPs
    :param width: width of the image if make_fixed_size is True
    :param height: height of the image if make_fixed_size is True
    :param x_margin: margin on the x axis if make_fixed_size is False
    :param y_margin: margin on the y axis if make_fixed_size is False
    '''

    #create images
    img_u, img_v, img_x = all_views_img_maker(all_TPs, channel_map, min_tps_to_create_img=min_tps_to_create_img, make_fixed_size=make_fixed_size, width=width, height=height, x_margin=x_margin, y_margin=y_margin)

    n_views = 0
    #show images
    if img_u[0, 0] != -1:
        n_views += 1
        plt.figure(figsize=(8, 20))
        plt.imshow(img_u)
        plt.show()
    if img_v[0, 0] != -1:
        n_views += 1
        plt.figure(figsize=(8, 20))
        plt.imshow(img_v)
        plt.show()
    if img_x[0, 0] != -1:
        n_views += 1
        plt.figure(figsize=(8, 20))
        plt.imshow(img_x)
        plt.show()

    if n_views > 1:
        fig = plt.figure()
        grid = ImageGrid(fig, 111,          # as in plt.subplot(111)
                        nrows_ncols=(1,3),
                        axes_pad=0.15,
                        share_all=True,
                        cbar_location="right",
                        cbar_mode="single",
                        cbar_size="7%",
                        cbar_pad=0.15,
                        )
        if img_u[0, 0] != -1:
            im = grid[0].imshow(img_u)
            grid[0].set_title('U plane')
        if img_v[0, 0] != -1:
            im = grid[1].imshow(img_v)
            grid[1].set_title('V plane')
        if img_x[0, 0] != -1:
            im = grid[2].imshow(img_x)
            grid[2].set_title('X plane')
        grid.cbar_axes[0].colorbar(im)
        grid.axes_llc.set_yticks(np.arange(0, img_u.shape[0], 100))
        plt.show()
    





def save_img(all_TPs, channel_map,save_path, outname='test', min_tps_to_create_img=2, make_fixed_size=False, width=500, height=1000, x_margin=10, y_margin=200):
    '''
    :param all_TPs: all trigger primitives in the event
    :param channel_map: channel map
    :param save_path: path where to save the images
    :param outname: base name for the images
    :param min_tps: minimum number of TPs to create the image
    :param make_fixed_size: if True, the image will have fixed size, otherwise it will be as big as the TPs
    :param width: width of the image if make_fixed_size is True
    :param height: height of the image if make_fixed_size is True
    :param x_margin: margin on the x axis if make_fixed_size is False
    :param y_margin: margin on the y axis if make_fixed_size is False
    '''

    #create images
    img_u, img_v, img_x = all_views_img_maker(all_TPs, channel_map, min_tps_to_create_img=min_tps_to_create_img, make_fixed_size=make_fixed_size, width=width, height=height, x_margin=x_margin, y_margin=y_margin)

    #save images
    if not os.path.exists(save_path):
        os.makedirs(save_path)

    n_views = 0

    if img_u[0, 0] != -1:
        n_views += 1
        plt.imsave(save_path + 'u_' + os.path.basename(outname) + '.png', img_u)
    if img_v[0, 0] != -1:
        n_views += 1
        plt.imsave(save_path+ 'v_' + os.path.basename(outname)+ '.png', img_v)
    if img_x[0, 0] != -1:
        n_views += 1
        plt.imsave(save_path +'x_' + os.path.basename(outname) + '.png', img_x)

    if n_views == 0:
        print('No images saved! Do a better clustering algorithm!')



    if n_views > 1:
        fig = plt.figure(figsize=(10, 26))
        grid = ImageGrid(fig, 111,          # as in plt.subplot(111)
                        nrows_ncols=(1,3),
                        axes_pad=0.5,
                        share_all=True,
                        cbar_location="right",
                        cbar_mode="single",
                        cbar_size="30%",
                        cbar_pad=0.25,
                        )   


        if img_u[0, 0] != -1:
            im = grid[0].imshow(img_u)
            grid[0].set_title('U plane')
        if img_v[0, 0] != -1:
            im = grid[1].imshow(img_v)
            grid[1].set_title('V plane')
        if img_x[0, 0] != -1:
            im = grid[2].imshow(img_x)
            grid[2].set_title('X plane')
        grid.cbar_axes[0].colorbar(im)
        grid.axes_llc.set_yticks(np.arange(0, img_u.shape[0], 100))
        # save the image
        plt.savefig(save_path+ 'multiview_' + os.path.basename(outname) + '.png')
        plt.close()




def create_dataset(clusters, channel_map, make_fixed_size=True, width=70, height=1000, x_margin=5, y_margin=50):
    '''
    :param clusters: list of clusters
    :param make_fixed_size: if True, the image will have fixed size, otherwise it will be as big as the TPs
    :param width: width of the image
    :param height: height of the image
    :param x_margin: margin on the x axis
    :param y_margin: margin on the y axis
    :return: dataset [[img],[label]] in numpy array format
    '''
    # create the full array beforehands
    # dataset_img = np.empty((len(clusters), height, width))
    # as above but with 3 channels
    dataset_img = np.zeros((len(clusters), height, width, 3), dtype=np.uint8)
    dataset_label = np.empty((len(clusters), 1), dtype=np.uint8)
    i=0
    for cluster in (clusters):
        # create the label
        # if len(np.unique(np.array(cluster)[:, 7])) > 1:
        #     label = 10
        # else:
        #     label = cluster[0][7]        
        if len(cluster) > 20:
            label = 1
        else:
            label = 0


        # append to the dataset as an array of arrays
        img_u, img_v, img_x = all_views_img_maker(np.array(cluster), channel_map, make_fixed_size=make_fixed_size, width=width, height=height, x_margin=x_margin, y_margin=y_margin)
        if img_u[0, 0] != -1:
            dataset_img[i, :, :, 0] = img_u
        if img_v[0, 0] != -1:
            dataset_img[i, :, :, 1] = img_v
        if img_x[0, 0] != -1:
            dataset_img[i, :, :, 2] = img_x
        
        dataset_label[i] = [label]
        
        i+=1

    return (dataset_img, dataset_label)


