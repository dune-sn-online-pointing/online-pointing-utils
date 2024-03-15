import numpy as np
import matplotlib.pyplot as plt
import sys

from image_creator import *
from utils import *
from cluster import *

np.random.seed(0)

def create_dataset_img(clusters, channel_map, min_tps_to_create_img=1, make_fixed_size=False, width=300, height=100, x_margin=2, y_margin=10, only_collection=True):
    '''
    :param clusters: list of clusters
    :param channel_map: array with the channel map
    :param min_tps_to_create_img: minimum number of tps to create an image
    :param make_fixed_size: if True, the images will have a fixed size
    :param width: width of the image
    :param height: height of the image
    :param x_margin: margin in the x direction
    :param y_margin: margin in the y direction
    :return: list of images
    '''
    if only_collection:
        dataset_img = np.empty((len(clusters), height, width, 1), dtype=np.uint16)
    else:
        dataset_img = np.empty((len(clusters), height, width, 3), dtype=np.uint16)

    for i, cluster in enumerate(clusters):
        if len(cluster) >= min_tps_to_create_img:
            # img = create_image(cluster.get_tps(), channel_map, make_fixed_size, width, height, x_margin, y_margin, only_collection)
            tps_to_draw = cluster.get_tps()
            tps_to_draw['time_start'] = tps_to_draw['time_start']%5000
            tps_to_draw['time_peak'] = tps_to_draw['time_peak']%5000

            img_u, img_v, img_x = create_images(tps_to_draw = tps_to_draw, channel_map = channel_map, make_fixed_size = make_fixed_size, width = width, height = height, x_margin = x_margin, y_margin = y_margin, only_collection = only_collection)
            if only_collection:
                dataset_img[i, :, :, 0] = img_x
            else:
                dataset_img[i, :, :, 0] = img_u
                dataset_img[i, :, :, 1] = img_v
                dataset_img[i, :, :, 2] = img_x
    return dataset_img


def create_dataset_label_process(clusters):
    '''
    :param clusters: list of clusters
    :return: list of labels
    '''
    dataset_label = np.empty(len(clusters), dtype=np.uint8)
    for i, cluster in enumerate(clusters):
        dataset_label[i] = cluster.get_true_label()
    return dataset_label    

def create_dataset_label_true_dir(clusters):
    '''
    :param clusters: list of clusters
    :return: list of labels
    '''
    dataset_label = np.empty((len(clusters), 3), dtype=np.float32)
    for i, cluster in enumerate(clusters):
        dataset_label[i] = cluster.get_true_dir()
    return dataset_label

def save_samples_from_ds(dataset, output_folder, name="img", n_samples=10):
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
    
    n_elements = dataset.shape[0] 
    indices = np.random.choice(n_elements, n_samples, replace=False)
    samples = dataset[indices]
    print(samples.shape)
    for i, sample in enumerate(samples):
        plt.figure(figsize=(5, 15))
        img = sample[:,:,0]
        print(img.shape)
        # print(np.unique(img))
        img = np.where(img == 0, np.nan, img)
        plt.imshow(img)
        # make ticks bigger
        plt.xticks(fontsize=20)
        plt.yticks(fontsize=20)
        # set labels
        plt.xlabel('Channel', fontsize=20)
        plt.ylabel('Time [ticks]', fontsize=20)

        plt.savefig(output_folder+name+str(i)+'.png')
        plt.clf()

        
    # fig= plt.figure(figsize=(20, 20))
    # for i, label in enumerate(labels_unique[0]):
    #     grid = ImageGrid(fig, 111,          # as in plt.subplot(111)
    #             nrows_ncols=(1,10),
    #             axes_pad=0.5,
    #             share_all=True,
    #             cbar_location="right",
    #             cbar_mode="single",
    #             cbar_size="30%",
    #             cbar_pad=0.25,
    #             )   
    #     indices = np.where(labels == label)[0]
    #     indices = indices[:np.minimum(n_samples_per_label, indices.shape[0])]
    #     samples = dataset[indices]
    #     # save the images
    #     plt.suptitle("Label: "+str(label), fontsize=25)

    #     for j, sample in enumerate(samples):
    #         im = grid[j].imshow(sample[:,:,0])
    #         grid[j].set_title(j)

    #     grid.cbar_axes[0].colorbar(im)
    #     grid.axes_llc.set_yticks(np.arange(0, sample.shape[0], 100))
    #     plt.savefig(output_folder+ 'all_'+str(label)+'.png')
    #     plt.clf()


