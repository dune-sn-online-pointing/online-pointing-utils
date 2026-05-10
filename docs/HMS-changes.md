## Changes 

# Change from "Event" to "event" in structure breaks almost everything.

New TP files have a different structure and more events that are out of order.
Can no longer assume event numbers are sequential.  Need to store in a map instead of a vector with event number as the index. 

# Add ability to read a list of files at both tpstream and tps order 

This allows reading files via streaming and documented inputs

Future change would allow running on the grid


So there are now 2 example files which do list readin and are needed to first process the background and then the signal

They are; 

'''
json/locallist_bg_only.json	
json/locallist_es_bg.json
'''


# had to patch the cluster_display as it was duplicating the Y axis multiple times and also squeezing the x axis every iteration. 

# remove opening and closing files for each event


## Todo

# implement run number in the trigger primitive object as we will need it. 

# Make maps of tp range for each event to allow faster access


## Trying to merge with algorithmic_3d_reco branch

### make new branch `merge_OSU_3d_reco` which follows `algorithmic_3d_reco`

- need to change Event to event to read production tuples
- replace the Input/Output module with the one from OSU to read file list
- copy in this file
- copy in the locallist json files and point to new area

Note that Utils.cpp has changed to utils.cpp 

- use `STANDARD_FORMAT` compile flag in `BackTracker.h` to switch to official trigger primitive directory structure. 

- add in maxcount argument to limit # of events you run over - still need to reimplement skip

- run int "six" problems, try to go back to python 3.9 instead of 3.13

- forced_face: int | None = None, in space_transformations.py requires 3.10

- make a new environment conda activate python-3.10 - that works. 

- can't find the background files in a folder.  May need to add explicit list (did it in add_background.cpp)

- seems to work now. 

## put in a pull request for merge_OSU_3d_reco

- still needs the Event/event resolved

- tried on Al9 but was unable to get file access to work. 

build sequence is

- check out the code

- source python/setup-python.sh

- ./scripts/compile.sh (sometimes need to remove build/CMakeCache.txt to get a clean build)

- problems with build in SL7 as it fails on the second test for setup because the ups install has gone away? 


