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
