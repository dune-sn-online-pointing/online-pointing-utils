import numpy as np
import sys
import matplotlib.pyplot as plt
import time
import os
import argparse
import ROOT
import create_images_from_tps_libs as tp2img


# write a parser to set up the running from command line
parser = argparse.ArgumentParser(description='Tranforms Trigger Primitives to images.')
parser.add_argument('--input_file', type=str, help='Input file name')
parser.add_argument('--show', action='store_true', help='show the image')
parser.add_argument('--save_img', action='store_true', help='save the image')
parser.add_argument('--save_ds', action='store_true', help='save the dataset')
parser.add_argument('--write', action='store_true', help='write the groups to a file')
parser.add_argument('--output_path', type=str, default='/eos/user/d/dapullia/tp_dataset/', help='path to save the image')
parser.add_argument('--img_save_folder', type=str, default='images/', help='folder to save the image')
parser.add_argument('--img_save_name', type=str, default='image', help='name to save the image')
parser.add_argument('--n_events', type=int, default=0, help='number of events to process')
parser.add_argument('--min_tps_to_group', type=int, default=4, help='minimum number of TPs to create a group')
parser.add_argument('--drift_direction', type=int, default=0, help='0 for horizontal drift, 1 for vertical drift')
parser.add_argument('--ticks_limit', type=int, default=150, help='closeness in ticks to group TPs')
parser.add_argument('--channel_limit', type=int, default=20, help='closeness in channels to group TPs')
parser.add_argument('--make_fixed_size', action='store_true', help='make the image size fixed')
parser.add_argument('--img_width', type=int, default=70, help='width of the image')
parser.add_argument('--img_height', type=int, default=1000, help='height of the image')
parser.add_argument('--x_margin', type=int, default=5, help='margin in x')
parser.add_argument('--y_margin', type=int, default=50, help='margin in y')
parser.add_argument('--min_tps_to_create_img', type=int, default=2, help='minimum number of TPs to create an image')
parser.add_argument('--preprocess_ema_ds', action='store_true', help='preprocess the ema dataset')
parser.add_argument('--use_sparse', action='store_true', help='use sparse matrices')


args = parser.parse_args()
filename = args.input_file
show = args.show
save_img = args.save_img
save_ds = args.save_ds
write = args.write
output_path = args.output_path
n_events = args.n_events
min_tps_to_group = args.min_tps_to_group
img_save_folder = args.img_save_folder
img_save_name = args.img_save_name
drift_direction = args.drift_direction
ticks_limit = args.ticks_limit
channel_limit = args.channel_limit
make_fixed_size = args.make_fixed_size
width = args.img_width
height = args.img_height
x_margin = args.x_margin
y_margin = args.y_margin
min_tps_to_create_img = args.min_tps_to_create_img
preprocess_ema_ds = args.preprocess_ema_ds
use_sparse = args.use_sparse

def read_root_file (filename):
    file = ROOT.TFile.Open(filename, "READ")

    # get tree name
    nrows = []
    event = []
    matrix = []

    for i in file.GetListOfKeys():
        # get the TTree
        tree = file.Get(i.GetName())
        print(f"Tree name: {tree.GetName()}")
        tree.Print()

        for entry in tree:
            # nrows = entry.NRows
            # event = entry.Event
            # matrix = entry.Matrix
            nrows.append(entry.NRows)
            event.append(entry.Event)
            # Matrix is <class cppyy.gbl.std.vector<vector<int> > at 0x16365890>
            # extract the matrix from the vector<vector<int>> object to numpy array
            m = np.empty((nrows[-1], 9), dtype=int)
            for j in range(nrows[-1]):
                for k in range(9):
                    m[j][k] = entry.Matrix[j][k]
            matrix.append(m)

    return nrows, event, matrix



if __name__=='__main__':

    #read first n_events lines in the data from txt, all are ints
    if n_events:
        all_TPs = np.loadtxt(filename, skiprows=0, max_rows=n_events, dtype=int)
    else:
        all_TPs = np.loadtxt(filename, skiprows=0, dtype=int)
    if preprocess_ema_ds:
        all_TPs = all_TPs[all_TPs[:,0].argsort()]   
        all_TPs[:,0] = all_TPs[:,0] + all_TPs[:,12]*4500
        all_TPs[:,2] = all_TPs[:,2] + all_TPs[:,12]*4500


    #read channel map
    channel_map = tp2img.create_channel_map_array(drift_direction=drift_direction)

    groups = tp2img.group_maker(all_TPs, channel_map, ticks_limit=ticks_limit, channel_limit=channel_limit, min_tps_to_group=min_tps_to_group)
    # groups = tp2img.group_maker_using_truth(all_TPs)
    print("Number of groups: ", len(groups))
    total_channels = channel_map.shape[0]
    n_views = np.unique(channel_map[all_TPs[:, 3]% total_channels, 1]).shape[0]
    print("Number of planes: ", n_views)

    # Prepare the output folder
    if save_img:
        if not os.path.exists(output_path+img_save_folder):
            os.makedirs(output_path+img_save_folder)

    if show or save_img:
        print("Creating images...")
        for i, group in enumerate(groups):
            if show:
                tp2img.show_img(np.array(group), channel_map, min_tps_to_create_img=min_tps_to_create_img, make_fixed_size=make_fixed_size, width=width, height=height, x_margin=x_margin, y_margin=y_margin)
            if save_img:
                tp2img.save_img(np.array(group), channel_map, save_path=output_path+img_save_folder, outname=img_save_name+str(i), min_tps_to_create_img=min_tps_to_create_img, make_fixed_size=make_fixed_size, width=width, height=height, x_margin=x_margin, y_margin=y_margin)
        print("Done!")
    if write:
        print("Writing groups to file...")
        with open(output_path+'groups.txt', 'w') as f:
            for i, group in enumerate(groups):
                f.write('Group'+str(i)+':\n')
                for tp in group:
                    f.write(f"{tp[0]} {tp[1]} {tp[2]} {tp[3]} {tp[4]} {tp[5]} {tp[6]} {tp[7]}\n")
                f.write('\n')
        print("Done!")

    if save_ds:
        print("Creating dataset in " + "dense" if not use_sparse else "sparse" + " format...")
        if not os.path.exists(output_path+'dataset/'):
            os.makedirs(output_path+'dataset/')
        dataset_img, dataset_label = tp2img.create_dataset(groups,  channel_map=channel_map, make_fixed_size=make_fixed_size, width=width, height=height, x_margin=x_margin, y_margin=y_margin, n_views=n_views, use_sparse=use_sparse)

        print("Dataset shape: ", dataset_img.shape)
        print("Dataset label shape: ", dataset_label.shape)
        np.save(output_path+'dataset/dataset_img.npy', dataset_img)
        np.save(output_path+'dataset/dataset_label.npy', dataset_label)

            
        print(np.unique(dataset_label,return_counts=True))
        print("Done!")
    print('Done!')


