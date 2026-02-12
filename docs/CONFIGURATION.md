# Configuration Guide

This guide provides comprehensive information about configuring the online-pointing-utils project.

## Configuration Overview

The project uses multiple configuration layers:

1. **[Parameter System](../parameters/PARAMETERS.md)** - Core constants and geometry
2. **[JSON Configurations](../json/README.md)** - Algorithm parameters and workflows  
3. **Environment Variables** - Runtime paths and options
4. **[Build Scripts](../scripts/README.md)** - Compilation and execution settings

## Parameter System Configuration

### Setting Up Parameters
```bash
# Set parameter directory
export PARAMETERS_DIR=/path/to/online-pointing-utils/parameters

# Or use the setup script
source setup_parameters.sh
```

### Parameter Files
- **[geometry.dat](../parameters/geometry.dat)** - Detector geometry constants
- **[timing.dat](../parameters/timing.dat)** - Time-related parameters
- **[conversion.dat](../parameters/conversion.dat)** - Unit conversion factors
- **[detector.dat](../parameters/detector.dat)** - Detector-specific settings
- **[analysis.dat](../parameters/analysis.dat)** - Analysis parameters

**Format Example:**
```
< geometry.apa_length_cm = 230.0 >    APA length in cm
< timing.time_tick_cm = 0.0805 >      Time tick in cm
```

**Detailed Documentation**: [parameters/PARAMETERS.md](../parameters/PARAMETERS.md)

## JSON Configuration Files

### Main Configuration Categories

#### Clustering Configurations
- **Location**: `json/cluster/`
- **Purpose**: Control clustering algorithm parameters
- **Example**: [json/cluster/example.json](../json/cluster/example.json)
- **Documentation**: [json/README.md](../json/README.md)

#### Analysis Configurations  
- **Location**: `json/analyze_tps/`, `json/analyze_clusters/`
- **Purpose**: Configure analysis workflows and cuts
- **Examples**: Various JSON files for different analysis scenarios

#### Data Conversion Configurations
- **Location**: `json/cluster_to_root/`, `json/clusters_to_dataset/`
- **Purpose**: Control data format conversion and output
- **Documentation**: [json/cluster_to_root/README.md](../json/cluster_to_root/README.md)

### JSON Configuration Structure
```json
{
  "filename": "input_file.root",
  "filelist": "list_of_files.txt", 
  "tick_limit": 3,
  "channel_limit": 1,
  "min_tps_to_cluster": 1,
  "adc_integral_cut_induction": 0,
  "adc_integral_cut_collection": 0
}
```

## Environment Variables

### Required Variables
- **`PARAMETERS_DIR`** - Path to parameter files directory
- **`HOME_DIR`** - Repository root directory (set by init.sh)
- **`SCRIPTS_DIR`** - Scripts directory path (set by init.sh)

### Optional Variables
- **`BUILD_DIR`** - Build directory path (default: `build/`)
- **`INIT_DONE`** - Environment initialization flag

### Setting Up Environment
```bash
# Automatic setup
source scripts/init.sh

# Manual setup
export PARAMETERS_DIR=/path/to/parameters
export HOME_DIR=/path/to/online-pointing-utils
```

## Build Configuration

### CMake Options
- **`USE_STATIC_LINKS`** - Enable static library linking
- **`CMAKE_BUILD_TYPE`** - Debug/Release build type

### Compilation Scripts
- **[compile.sh](../scripts/README.md)** - Main compilation script
- **Options**: `--no-compile`, `--clean-compile`

## Workflow Configuration

### Analysis Workflows
Each analysis type has its own script and configuration:

```bash
# Trigger primitive analysis
./scripts/analyze_tps.sh -j json/analyze_tps/config.json

# Clustering workflow  
./scripts/cluster.sh -j json/cluster/config.json

# Backtracking analysis
./scripts/backtrack.sh -j json/backtrack/config.json
```

### Configuration File Discovery
Scripts automatically locate configuration files:
```bash
# Searches multiple locations for config.json
./scripts/cluster.sh -j config.json
```

## Advanced Configuration

### Custom Parameter Files
Create custom parameter files following the format:
```
*******************************************************************************
Custom Parameters
*******************************************************************************

< custom.parameter = value >    Description
```

### Configuration Validation
The system provides validation for:
- Missing parameter files (warnings)
- Missing parameters (runtime exceptions)  
- Invalid JSON syntax (parse errors)
- Required vs optional configuration fields

### Debugging Configuration
```bash
# Enable verbose mode in scripts
./scripts/cluster.sh -j config.json -v

# Check parameter loading
echo $PARAMETERS_DIR
ls -la $PARAMETERS_DIR/*.dat
```

## Migration and Compatibility

### Legacy Configuration
- Old hardcoded constants are still supported via compatibility macros
- Gradual migration path from `.h` files to `.dat` files
- Backward compatibility maintained for existing code

### Upgrading Configuration
1. Replace hardcoded constants with parameter file values
2. Update JSON configurations for new algorithm parameters  
3. Set environment variables in deployment scripts
4. Test configuration loading with validation tools

---

For specific configuration examples and troubleshooting, see the linked README files and parameter documentation.