## Changes 

Change from "Event" to "event" in structure breaks almost everyting.

New TP files have a different structure and more events that are out of order.
Can no longer assume event numbers are sequential.  Need to store in a map instead of a vector with event number as the index. 

change the read method for the background overlay to be the same as for the original marley events.  Requires passing the tree path as it is different.
Also may need disabling the matching which is done on the read for the signal.

required making a tps_map driver which might actually be better for the original read-in

Turns out I should have run the background through the normal tp stream maker. 

## Todo

implement run number in the trigger primitive object