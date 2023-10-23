# Online Pointing Utilities Scripts

This directory contains scripts that are used to run the utilities.

## Create images from TPs

The script `create-images-from-tps.sh` creates images from TPs. 

### Usage
You can run the script setting the input file and the output folder with:
```
./create_images_from_tps.py  -i <input_file> -o <output_folder> [-h]
```

For the moment, to set all the parameters you need to edit the script changing the values of the variables at the beginning of the script.
The basic output settings are:
- `SHOW`: if show, the images are shown in a window runtime
- `SAVE_IMG`: if save_img, the images are saved in a folder
- `SAVE_DS`: if save_ds, the images are saved in a numpy array
- `WRITE`: if write, the groups are written in a text file

## HDF5 converter

The script `convert-hdf5-to-text.sh` converts the HDF5 files from the DUNE DAQ to a more readable format.

### Usage


You can run the script setting the input file and the output folder with:
```
./convert-hdf5-to-text.sh  -i <input_file> -o <output_folder> [-h]
```

For the moment, to set all the parameters you need to edit the script changing the values of the variables at the beginning of the script.
You can change the output format by changing the value of the variable `FORMAT`. The possible values are:
- `txt`: the output is a text file
- `npy`: the output is a numpy array
- `img_groups`: TPs are grouped, and an image is produced for each group
- `img_all`: an image with all the tps is produced. You can also set the time range of the image with the variables `TIME_START_FOR_IMG_ALL` and `TIME_END_FOR_IMG_ALL`.

**NOTE 1**: this script relis on the DUNE DAQ software. You need to have it installed and sourced before running the script. You can source it with:
```
source setup-dune-daq.sh
```

**NOTE 2**: if you want to produce images from the TPs, you need to have a local installation of matplotlib. In my experience, you have to use the NP04 machines to do that. You can install it with:
```
pip install --prefix=$PATH_TO_INSTALL matplotlib
```
and then modify the `PYTHONPATH` variable to include the path to the installation folder.