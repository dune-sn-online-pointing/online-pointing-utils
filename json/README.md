# JSON Configuration Files

JSON files in this directory contain configuration for the analysis pipeline. They are global and valid for all apps in the `scripts/` directory.

## Tracked Example Files

The following example configurations are tracked in git to help you get started:

- **example_es_clean.json**: Electron-scattering signal without backgrounds  
- **example_es_bg.json**: ES signal with background mixing (higher cuts)  
- **example_bg.json**: Background-only analysis

All other JSON files in this directory are **gitignored** to avoid tracking your personal configurations or production settings.

## Usage Workflow

1. **Copy an example**: 
   ```bash
   cp json/example_es_clean.json json/my_analysis.json
   ```

2. **Edit paths**: Update the folder paths to point to your data  
   - `signal_folder`: Base folder containing `tpstreams/` subdirectory with signal ROOT files
   - `bg_folder`: Base folder containing `tpstreams/` subdirectory with background ROOT files  
   - `main_folder`: Alternative to `signal_folder` for background-only runs

3. **Run the pipeline**:
   ```bash
   ./scripts/sequence.sh --json json/my_analysis.json --all
   ```

## Base Folder Structure

Your data folders **must** contain a `tpstreams/` subfolder:

```
/path/to/your/signal/data/
└── tpstreams/
    ├── file1_tpstream.root
    ├── file2_tpstream.root
    └── ...
```

The pipeline automatically discovers files within `tpstreams/` based on the base folder you provide.

## Path Override Keys

You can override specific input paths instead of using folder discovery:

- `tpstream_input_file`: Direct path to a single tpstream ROOT file or list file
- `tps_input_file`: Direct path to backtracked TPs file
- `tps_bg_folder` or `tps_bg_input_file`: Override background TPs location
- `clusters_input_file` or `clusters_folder`: Override clustered data location

## Common Configuration Knobs

### Clustering Parameters
- `tick_limit`: Maximum time difference (ticks) for TPs in same cluster
- `channel_limit`: Maximum channel difference for TPs in same cluster  
- `min_tps_to_cluster`: Minimum TPs required to form a cluster
- `tot_cut`: Time-over-threshold ADC cut (higher for backgrounds)
- `energy_cut`: Minimum energy cut in MeV (higher for backgrounds)

### Backtracking & Truth Matching
- `backtracker_error_margin`: Tolerance for TP-truth association (channels/ticks)
- `time_tolerance_ticks`: Time window for TP-SimIDE matching
- `spatial_tolerance_cm`: Spatial tolerance for position matching

### File Control
- `max_files`: Limit number of input files to process (-1 for all)
- `skip_files`: Number of files to skip at the start
- `products_prefix`: Prefix for output folder names (e.g., "es_clean" → "es_clean_clusters_tick3_ch2_...")

### Output Folders
- `outputFolder`: Base output folder for products (default: "data/")
- `reports_folder`: Folder for analysis reports and plots

## Tips

- **Clean signal analysis**: Use low cuts (tot=1, energy=0.0)  
- **Signal + backgrounds**: Use higher cuts (tot=3, energy=2.0) to reduce noise
- **Prefix naming**: Use descriptive prefixes to distinguish different analysis configurations
- **Test with few files**: Set `max_files: 10` when testing new configurations
 