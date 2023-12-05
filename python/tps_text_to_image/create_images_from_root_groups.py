import numpy as np
import sys
import matplotlib.pyplot as plt
import time
import os
import argparse
import ROOT

import create_images_from_tps_libs as tp2img


parser = argparse.ArgumentParser(description='Tranforms Trigger Primitives to images.')
parser.add_argument('--input_file', type=str, help='Input file name')
parser.add_argument('--show', action='store_true', help='show the image')
parser.add_argument('--save_img', action='store_true', help='save the image')
parser.add_argument('--save_ds', action='store_true', help='save the dataset')
parser.add_argument('--write', action='store_true', help='write the groups to a file')
parser.add_argument('--output_path', type=str, default='/eos/user/d/dapullia/tp_dataset/', help='path to save the image')
parser.add_argument('--img_save_folder', type=str, default='images/', help='folder to save the image')
parser.add_argument('--img_save_name', type=str, default='image', help='name to save the image')
parser.add_argument('--make_fixed_size', action='store_true', help='make the image size fixed')
parser.add_argument('--img_width', type=int, default=70, help='width of the image')
parser.add_argument('--img_height', type=int, default=100, help='height of the image')
parser.add_argument('--x_margin', type=int, default=5, help='margin in x')
parser.add_argument('--y_margin', type=int, default=10, help='margin in y')
parser.add_argument('--min_tps_to_create_img', type=int, default=2, help='minimum number of TPs to create an image')
parser.add_argument('--drift_direction', type=int, default=0, help='0 for horizontal drift, 1 for vertical drift')
parser.add_argument('--use_sparse', action='store_true', help='use sparse matrices')
parser.add_argument('--min_tps_to_group', type=int, default=3, help='minimum number of TPs to create a group')


args = parser.parse_args()
filename = args.input_file
show = args.show
save_img = args.save_img
save_ds = args.save_ds
write = args.write
output_path = args.output_path
img_save_folder = args.img_save_folder
img_save_name = args.img_save_name
make_fixed_size = args.make_fixed_size
width = args.img_width
height = args.img_height
x_margin = args.x_margin
y_margin = args.y_margin
min_tps_to_create_img = args.min_tps_to_create_img
drift_direction = args.drift_direction
use_sparse = args.use_sparse
min_tps_to_group = args.min_tps_to_group


def read_root_file (filename, min_tps_to_group=1):
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
            if entry.NRows < min_tps_to_group:
                continue
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

bkg_details = {
    "kUnknown": 0,
    "kMarley": 1,
    "kAr39GenInLAr": 2,
    "kKr85GenInLAr": 3,
    "kAr42GenInLAr": 4,
    "kK42From42ArGenInLAr": 5,
    "kRn222ChainRn222GenInLAr": 6,
    "kRn222ChainPo218GenInLAr": 7,
    "kRn222ChainPb214GenInLAr": 8,
    "kRn222ChainBi214GenInLAr": 9,
    "kRn222ChainPb210GenInLAr": 10,
    "kK40GenInCPA": 11,
    "kU238ChainGenInCPA": 12,
    "kK42From42ArGenInCPA": 13,
    "kRn222ChainPo218GenInCPA": 14,
    "kRn222ChainPb214GenInCPA": 15,
    "kRn222ChainBi214GenInCPA": 16,
    "kRn222ChainPb210GenInCPA": 17,
    "kRn222ChainFromBi210GenInCPA": 18,
    "kCo60GenInAPA": 19,
    "kU238ChainGenInAPA": 20,
    "kRn222ChainGenInPDS": 21, # //// now called 7
    "kNeutronGenInRock": 22
}

bkg_aggregate = {
    "kUnknown": 0,
    "kMarley": 1,
    "kU238ChainGenInAPA": 2,
    "kCo60GenInAPA": 2,
    "kK40GenInCPA": 3,
    "kU238ChainGenInCPA": 3,
    "kK42From42ArGenInCPA": 3,
    "kRn222ChainPo218GenInCPA": 3,
    "kRn222ChainPb214GenInCPA": 3,
    "kRn222ChainBi214GenInCPA": 3,
    "kRn222ChainPb210GenInCPA": 3,
    "kRn222ChainFromBi210GenInCPA": 3,
    "kAr39GenInLAr": 4,
    "kNeutronGenInRock": 5,
    "kKr85GenInLAr": 6,
    "kPo210GenInLAr": 7, # //// not existing anymore
    "kRn222ChainRn222GenInLAr": 8,
    "kRn222ChainPo218GenInLAr": 8,
    "kRn222ChainPb214GenInLAr": 8,
    "kRn222ChainBi214GenInLAr": 8,
    "kRn222ChainPb210GenInLAr": 8,
    "kAr42GenInLAr": 9,
    "kK42From42ArGenInLAr": 9,
}

dict_lab = {
    0: 0,
    1: 1,
    2: 4,
    3: 6,
    4: 9,
    5: 9,
    6: 8,
    7: 8,
    8: 8,
    9: 8,
    10: 8,
    11: 3,
    12: 3,
    13: 3,
    14: 3,
    15: 3,
    16: 3,
    17: 3,
    18: 3,
    19: 2,
    20: 2,
    21: 7,
    22: 5
}


if __name__=='__main__':


    # # quick hack
    # tps = np.loadtxt("/eos/home-e/evilla/dune/sn-data/tpstream_standardHF_thresh30_nonoise_newLabels_23evts.txt", dtype=int)

    # print(np.unique(tps[:,11], return_counts=True))



    channel_map = tp2img.create_channel_map_array(drift_direction=drift_direction)

    print("Reading root file...")
    # nrows, event, matrix = read_root_file(filename, min_tps_to_group=min_tps_to_group)
    # groups = np.array(matrix, dtype=object)
    # n_views = 1
    # hack to get multiple files, I want 1 groups object
    for i in range(8):
        name = f"/eos/user/d/dapullia/tp_dataset/emaprod/new_lab_multirun/{i}/X/groups_tick_limits_10_channel_limits_2_min_tps_to_group_1.root"
        nrows, event, matrix = read_root_file(name, min_tps_to_group=min_tps_to_group)
        print(len(matrix))
        if i == 0:
            groups = np.array(matrix, dtype=object)
        else:
            groups = np.append(groups, np.array(matrix, dtype=object))
    n_views = 1

    # channs = np.array([], dtype=int)
    # for i in range(100):
    #     channs = np.append(channs, np.unique(groups[i][:,3]))
    # print(channs)
    # # check in the channel map if the channels are in the same plane    
    # print(channel_map[channs%2560])
    # # for i in range(100):

    print("Done: ", len(groups), " groups read!")

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
        dataset_img, dataset_label = tp2img.create_dataset(groups,  channel_map=channel_map, make_fixed_size=make_fixed_size, width=width, height=height, x_margin=x_margin, y_margin=y_margin, n_views=n_views, use_sparse=use_sparse, unknown_label=99, idx=6, dict_lab=dict_lab)

        print("Dataset shape: ", dataset_img.shape)
        print("Dataset label shape: ", dataset_label.shape)
        np.save(output_path+'dataset/dataset_img.npy', dataset_img)
        np.save(output_path+'dataset/dataset_label.npy', dataset_label)

            
        print(np.unique(dataset_label,return_counts=True))
        print("Done!")
    print('Done!')


