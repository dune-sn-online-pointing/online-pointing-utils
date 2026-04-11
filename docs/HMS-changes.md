## Changes 

Change from "Event" to "event" in structure breaks almost everyting.

New TP files have a different structure and more events that are out of order.
Can no longer assume event numbers are sequential.  Need to store in a map instead of a vector with event number as the index. 


Turns out I should have run the background through the normal tp stream maker. 

So there are now 2 example files which do list readin and are needed to first process the background and then the signal

They are; 

'''
json/locallist_bg_only.json	
json/locallist_es_bg.json
'''

had to patch the cluster_display as it was duplicating the Y axis multiple times and also squeezing the x axis every iteration. 

## Todo

implement run number in the trigger primitive object as we will need it. 
