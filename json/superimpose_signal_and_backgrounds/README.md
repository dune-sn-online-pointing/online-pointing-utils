More verbose explanation of the json config file:

```json
{
    "bkg_filenames": "",                // Path to the file containing the list of background files
    "signal_clusters": "",              // Path to the signal clusters file
    "output_folder": "",                // Directory where the output files will be saved
    "radius": 50,                       // Radius parameter for the volume
    "plane": 2,                         // Plane number for clustering. 0, 1 or 2 (U, V, X)
    "max_events_per_filename": 1000     // Maximum number of events per output file
}
```