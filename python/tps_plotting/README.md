# tp-plotting

These are simple tools to display Trigger Primitives.
They will be moved to `dune-daq`, leaving out the parts referring to offline.

### Requirements

You just need matplotlib and numpy.

### Input files
For textfiles, the idea is that the input files contain the TP variables in this order:
```
time_start, time_over_threshold, time_peak, channel, adc_integral, adc_peak, detid, type, algorithm, version, flags
```
If there are more variables after these (MC truth from offline), it's not a problem, they will just be ignored.

The example files in the repo come from offline, but are ok for testing.

The code is getting ready to accept hdf5 files as input.

### Usage

The notebook should be self-explanatory, and can be good for quick checks.

`plot-tp-properties.py` does the same thing; the usage can be seen with `-h`:

```
usage: plot-tp-properties.py [-h] [-f FILES [FILES ...]] [-e OUTPUT_FOLDER] [-n NUMBER_TPS] [-s] [-a] [-t] [-o] [-i] [-p] [-c] [-d]

options:
  -h, --help            show this help message and exit
  -f FILES [FILES ...], --files FILES [FILES ...]
                        files to read
  -e OUTPUT_FOLDER, --output-folder OUTPUT_FOLDER
                        output folder
  -n NUMBER_TPS, --number-tps NUMBER_TPS
                        number of tps to plot
  -s, --superimpose     superimpose plots
  -a, --all             plot all the TP variables
  -t, --time-peak       plot time peak
  -o, --time-over-threshold
                        plot time over threshold
  -i, --adc-integral    plot adc integral
  -p, --adc-peak        plot adc peak
  -c, --channel         plot channel
  -d, --detid           plot detid
```

You can change some display options for each variable directly inside the python script.
Default location for output is `./output/`, you see the folder in the repo but its content is added to `.gitignore`.


### Include

All functions are in `include/PlottingUtils.py`. 
There will be a separate lib for offline tools.