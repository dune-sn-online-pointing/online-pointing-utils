# API Reference

This document provides quick reference links to key APIs and interfaces in the online-pointing-utils project.

## Core Data Structures

### TriggerPrimitive
- **Location**: `src/objects/TriggerPrimitive.hpp`
- **Description**: Represents individual trigger primitives with timing, channel, and ADC information
- **Key Methods**: `GetTimeStart()`, `GetDetectorChannel()`, `GetAdcIntegral()`, `GetView()`

### Cluster
- **Location**: `src/objects/Cluster.h`
- **Description**: Collection of trigger primitives forming a cluster
- **Key Methods**: `get_tps()`, `get_reco_pos()`, `get_total_charge()`, `get_size()`

### TrueParticle
- **Location**: `src/objects/TrueParticle.h`
- **Description**: Monte Carlo truth information for particles
- **Usage**: Truth matching and validation

## Algorithm Libraries

### Backtracking Functions
- **Location**: `src/backtracking/Backtracking.h`
- **Key Functions**:
  - `calculate_position()` - Calculate 3D position from trigger primitive
  - `eval_y_knowing_z_U_plane()` - Y-coordinate calculation for U-plane
  - `eval_y_knowing_z_V_plane()` - Y-coordinate calculation for V-plane

### Clustering Algorithms
- **Location**: `src/clusters/Clustering.h`
- **Key Functions**:
  - `channel_condition_with_pbc()` - Channel proximity with periodic boundary conditions
  - `make_cluster()` - Main clustering algorithm
  - `write_clusters()` / `write_clusters_with_match_id()` - ROOT output

### Volume Operations
- **Location**: `src/clusters/AggregateClustersWithinVolume.h`
- **Key Functions**:
  - `aggregate_clusters_within_volume()` - Spatial cluster aggregation
  - **Location**: `src/clusters/CreateVolumeClusters.h`
  - `get_tps_around_cluster()` - Find trigger primitives near cluster

## Parameter System

### ParametersManager
- **Location**: `src/lib/ParametersManager.h`
- **Usage**: 
- **Usage**:
  ```cpp
  #include "ParametersManager.h"
  double value = GET_PARAM_DOUBLE("geometry.apa_length_cm");
  ```
- **Documentation**: parameter files under `parameters/*.dat`

### Legacy Constants
- **Location**: `src/lib/Utils.h`
- **Description**: Backward-compatible access to parameters
- **Usage**: Direct constants like `apa_lenght_in_cm`, `wire_pitch_in_cm_collection`

## Utility Functions

### Global Utilities
- **Location**: `src/lib/Global.h`
- **Functions**: General utility functions used across the project

### Geometry Constants
- **Location**: `src/lib/Utils.h`
- **Namespaces**: `APA::` (detector configuration), `PDG::` (particle codes)

## Application Interfaces

### Main Applications
Located in `src/app/`:
- `make_clusters.cpp`
- `backtrack_tpstream.cpp`
- `add_backgrounds.cpp`
- `match_clusters.cpp`
- `analyze_tps.cpp`
- `analyze_clusters.cpp`

### Script Interfaces
Located in `scripts/`:
- **make_clusters.sh** - Clustering workflow script
- **backtrack.sh** - Backtracking workflow script
- **analyze_tps.sh** - TP analysis workflow script

See [APPS.md](APPS.md) for an up-to-date overview.

## Configuration Interfaces

### JSON Configuration
- **Location**: `json/` directory
- **Documentation**: [../json/README.md](../json/README.md)
- **Usage**: Algorithm parameters, file paths, processing options

### Parameter Files
- **Location**: `parameters/` directory
- **Format**: `.dat` files with `< key = value >` syntax
- **Documentation**: parameter files under `parameters/*.dat`

## External Dependencies

### Submodules
- **simple-cpp-logger**: [README](../submodules/simple-cpp-logger/README.md)
- **simple-cpp-cmd-line-parser**: [README](../submodules/simple-cpp-cmd-line-parser/README.md)
- **cpp-generic-toolbox**: [README](../submodules/cpp-generic-toolbox/README.md)

### ROOT Framework
- **Usage**: Data I/O, histograms, file formats
- **Integration**: Via CMake configuration and ROOT libraries

---

For implementation details and examples, see the corresponding README files linked above and the source code documentation.