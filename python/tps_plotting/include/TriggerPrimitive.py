import numpy as np
from dataclasses import dataclass
import datetime

#-------------------------------------------
# Constants
#-------------------------------------------

# these are taken from https://github.com/DUNE-DAQ/trgdataformats/blob/develop/include/trgdataformats/Types.hpp
# TODO change data types to proper C++-defined ones (uint32_t, etc)

INVALID_TIMESTAMP = datetime.datetime.max   # uint64_t
INVALID_CHANNEL = 0xFFFFFFFF                # uint32_t
INVALID_VERSION = 0XFFF                     # uint16_t
INVALID_DETIT = 0XFFF                       # uint16_t

#-------------------------------------------
# TriggerPrimitive struct
#-------------------------------------------

# format consistent with the header TriggerPrimitive.hpp: 
# https://github.com/DUNE-DAQ/trgdataformats/blob/develop/include/trgdataformats/TriggerPrimitive.hpp, 
# as of 2023-11-08
class TriggerPrimitive:
    def __init__(self,
                 time_start=INVALID_TIMESTAMP,
                 time_peak=INVALID_TIMESTAMP,
                 time_over_threshold=INVALID_TIMESTAMP,
                 channel=INVALID_CHANNEL,
                 adc_integral=0,
                 adc_peak=0,
                 detid=INVALID_DETIT,
                 type=0,
                 algorithm=0,
                 version=1,
                 flags=0):
        
        # DAQ TP variables
        self.time_start = time_start
        self.time_peak = time_peak
        self.time_over_threshold = time_over_threshold
        self.channel = channel
        self.adc_integral = adc_integral
        self.adc_peak = adc_peak
        self.detid = detid
        self.type = type
        self.algorithm = algorithm
        self.version = version
        self.flags = flags
