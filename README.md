# Online-pointing-utils

This repo contains a collection of tools that are being used for the online supernova pointing project.
It includes some utilities to plot the Trigger Primitives, to handle samples and merge them together, to prepare datasets to be used in other stages.

## Structure

In `inc/` and `src/` there are some libraries that are called by the apps in `app/`.
You can create your own apps starting from the functions in the libraries.

Under `python` there are some python libraries. 
There are some duplicates of functions that were then rewritten in cpp to be faster.

Under `scripts/` you can find bash scripts to run the apps or a series of operations.
If you add more, make sure to set them up without local paths (which are supposed to be written only in json settings files).

## Install and use

To install the repo, clone it: 

```
git clone https://github.com/dune-sn-online-pointing/online-pointing-utils.git
```

Whenever running any of the scripts, the code is automatically compiled.
If you want to compile the code manually, go to `build/` and do

```
cmake ../; make -j $(nproc)
```

If the machine you are working on meets the requirements, the process will complete (there might be non-fatal warnings).
Under `build/app/` you will find the executables. 
Just doing `./cluster_to_root` will display their usage, meaning what arguments they require. 
The apps can also be run through bash scripts under `scripts/`, that will have the same name.


