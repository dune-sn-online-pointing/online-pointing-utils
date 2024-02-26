import numpy as np
import argparse
import os

seed = 42
np.random.seed(seed)

parser = argparse.ArgumentParser()
parser.add_argument("--datasets_img", help="input data file", default="", type=str)
parser.add_argument("--datasets_process_label", help="input process label file", default="", type=str)
parser.add_argument("--datasets_true_dir_label", help="input true direction label file", default="", type=str)
parser.add_argument("--output_folder", help="save path", default="", type=str)
parser.add_argument("--remove_process_labels", help="remove labels", default=[], nargs='+', type=int)
parser.add_argument("--shuffle", help="shuffle data", default=1, type=int)
parser.add_argument("--balance", help="balance data", default=1, type=int)

args = parser.parse_args()
datasets_img = args.datasets_img
datasets_process_label = args.datasets_process_label
datasets_true_dir_label = args.datasets_true_dir_label
output_folder = args.output_folder
remove_process_labels = args.remove_process_labels
shuffle = args.shuffle
balance = args.balance

if __name__ == '__main__':
    # read data and labels    
    datasets_img = np.loadtxt(datasets_img, dtype=str)
    datasets_process_label = np.loadtxt(datasets_process_label, dtype=str)
    datasets_true_dir_label = np.loadtxt(datasets_true_dir_label, dtype=str)

    # check if data contains at least one element
    if len(datasets_img) < 1:
        print("No dataset image found!")
        exit()
    # check if data and labels have different lengths
    mix_process_label = True
    mix_true_dir_label = True

    if len(datasets_img) != len(datasets_process_label):
        print("data and labels have different lengths!")
        mix_process_label = False
    if len(datasets_img) != len(datasets_true_dir_label):
        print("data and labels have different lengths!")
        mix_true_dir_label = False
    
    if not (mix_process_label or mix_true_dir_label):
        print("No label found!")
        exit()
    
    # create the output folder
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    ds_img = []
    ds_label_process = []
    ds_label_true_dir = []

    for i in range(len(datasets_img)):
        print(i)
        print("loading dataset: ", datasets_img[i])
        dataset_img = np.load(datasets_img[i], allow_pickle=True)
        print("Shape of the dataset_img: ", dataset_img.shape)
        ds_img.append(dataset_img)
        if mix_process_label:
            print("loading process label: ", datasets_process_label[i])
            if datasets_process_label[i].isdigit():
                process_labelset = np.full((len(dataset_img),1), int(datasets_process_label[i]))
            else:
                process_labelset = np.load(datasets_process_label[i])
            ds_label_process.append(process_labelset)
        if mix_true_dir_label:
            print("loading true direction label: ", datasets_true_dir_label[i])
            true_dir_labelset = np.load(datasets_true_dir_label[i])
            ds_label_true_dir.append(true_dir_labelset)
    
    # concatenate the datasets
    dataset_img = np.concatenate(ds_img, axis=0)
    if mix_process_label:
        dataset_label_process = np.concatenate(ds_label_process, axis=0)
    if mix_true_dir_label:
        dataset_label_true_dir = np.concatenate(ds_label_true_dir, axis=0)

    # remove labels
    if remove_process_labels:
        mask = np.isin(dataset_label_process, remove_process_labels)
        mask = np.reshape(mask, (len(mask)))
        dataset_label_process = dataset_label_process[~mask]
        dataset_img = dataset_img[~mask]
        if mix_true_dir_label:
            dataset_label_true_dir = dataset_label_true_dir[~mask]
    
    # balance the data
    if balance and mix_process_label:
        print("Balancing the data")
        unique, counts = np.unique(dataset_label_process, return_counts=True)
        print("Unique: ", unique)
        print("Counts: ", counts)
        min_count = np.min(counts)
        print("Min number of events: ", min_count)
        balanced_dataset_img = []
        balanced_dataset_label_process = []
        balanced_dataset_label_true_dir = []
        for i in range(len(unique)):
            indices = np.where(dataset_label_process == unique[i])[0]
            np.random.shuffle(indices)
            indices = indices[:min_count]
            balanced_dataset_img.append(dataset_img[indices])
            balanced_dataset_label_process.append(dataset_label_process[indices])
            if mix_true_dir_label:
                balanced_dataset_label_true_dir.append(dataset_label_true_dir[indices])
        dataset_img = np.concatenate(balanced_dataset_img, axis=0)
        dataset_label_process = np.concatenate(balanced_dataset_label_process, axis=0)
        if mix_true_dir_label:
            dataset_label_true_dir = np.concatenate(balanced_dataset_label_true_dir, axis=0)

    # shuffle the data
    if shuffle:
        print("Shuffling the data")
        indices = np.arange(len(dataset_img))
        np.random.shuffle(indices)
        dataset_img = dataset_img[indices]
        if mix_process_label:
            dataset_label_process = dataset_label_process[indices]
        if mix_true_dir_label:
            dataset_label_true_dir = dataset_label_true_dir[indices]

    # save the data
    print("Saving the data")
    print("Shape of the dataset_img: ", dataset_img.shape)
    np.save(output_folder + 'dataset_img.npy', dataset_img)
    if mix_process_label:
        np.save(output_folder + 'dataset_label_process.npy', dataset_label_process)
    if mix_true_dir_label:
        np.save(output_folder + 'dataset_label_true_dir.npy', dataset_label_true_dir)


