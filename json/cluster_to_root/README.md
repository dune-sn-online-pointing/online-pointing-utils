# Configuration Parameters

The following parameters can be configured in the JSON file:

- `filename`: Path to a file containing a list of input files.
- `output_folder`: Path to the output folder.
- `supernova_option`: This controls if the SN tps are read from file: 
    - `0`: both SN and bkg
    - `1`: only SN
    - `2`: only bkg
- `main_track_option`: This controls the main track selection:
    - `0`: keep all
    - `1`: keep only the main track
    - `2`: keep everything but the main track
    - `3`: keep everything but assign different labels to main track and other tracks
- `tick_limit`: This controls the proximity condition on the ticks dimension.
- `channel_limit`: This controls the proximity condition on the channels dimension.
- `min_tps_to_cluster`: This controls the minimum number of tps to form a cluster.
- `plane`: This controls the plane to be used for the clustering:
    - `0`: U
    - `1`: V
    - `2`: X
    - `3`: all
- `max_events_per_filename`: This controls the maximum number of events to be processed per input file.
- `adc_integral_cut`: This controls the cut on the integral of the ADC values.
- `use_electron_direction`: This controls the use of the electron direction as true direction.
