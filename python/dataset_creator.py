import numpy as np
import matplotlib.pyplot as plt
import sys

from image_creator import *
from utils import *
from group import *


def create_dataset_img(groups, channel_map, min_tps_to_create_img=1, make_fixed_size=False, width=300, height=100, x_margin=2, y_margin=10, only_collection=True):
    '''
    :param groups: list of groups
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
        dataset_img = np.empty((len(groups), height, width, 1), dtype=np.uint16)
    else:
        dataset_img = np.empty((len(groups), height, width, 3), dtype=np.uint16)

    for i, group in enumerate(groups):
        if len(group) >= min_tps_to_create_img:
            # img = create_image(group.get_tps(), channel_map, make_fixed_size, width, height, x_margin, y_margin, only_collection)
            img_u, img_v, img_x = create_images(tps_to_draw = group.get_tps(), channel_map = channel_map, make_fixed_size = make_fixed_size, width = width, height = height, x_margin = x_margin, y_margin = y_margin, only_collection = only_collection)
            if only_collection:
                dataset_img[i, :, :, 0] = img_x
            else:
                dataset_img[i, :, :, 0] = img_u
                dataset_img[i, :, :, 1] = img_v
                dataset_img[i, :, :, 2] = img_x
    return dataset_img

def create_dataset_label_process(groups):
    '''
    :param groups: list of groups
    :return: list of labels
    '''
    dataset_label = np.empty(len(groups), dtype=np.uint8)
    for i, group in enumerate(groups):
        dataset_label[i] = group.get_true_label()
    return dataset_label    

def create_dataset_label_true_dir(groups):
    '''
    :param groups: list of groups
    :return: list of labels
    '''
    dataset_label = np.empty((len(groups), 3), dtype=np.float32)
    for i, group in enumerate(groups):
        dataset_label[i] = group.get_true_dir()
    return dataset_label

