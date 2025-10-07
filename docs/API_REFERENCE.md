# API Reference

This document provides quick reference links to key APIs and interfaces in the online-pointing-utils project.

## Core Data Structures

### TriggerPrimitive
- **Location**: `src/objects/inc/TriggerPrimitive.hpp`
- **Description**: Represents individual trigger primitives with timing, channel, and ADC information
- **Key Methods**: `GetTimeStart()`, `GetDetectorChannel()`, `GetAdcIntegral()`, `GetView()`

### Cluster
- **Location**: `src/objects/inc/Cluster.h`
- **Description**: Collection of trigger primitives forming a cluster
- **Key Methods**: `get_tps()`, `get_reco_pos()`, `get_total_charge()`, `get_size()`

### TrueParticle
- **Location**: `src/objects/inc/TrueParticle.h`
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
- **Location**: `src/clustering/inc/cluster_to_root_libs.h`
- **Key Functions**:
  - `cluster_maker()` - Main clustering algorithm
  - `write_clusters_to_root()` - ROOT file output
  - `channel_condition_with_pbc()` - Channel proximity with periodic boundary conditions

### Volume Operations
- **Location**: `src/clustering/inc/aggregate_clusters_within_volume_libs.h`
- **Key Functions**:
  - `aggregate_clusters_within_volume()` - Spatial cluster aggregation
  - **Location**: `src/clustering/inc/create_volume_clusters_libs.h`
  - `get_tps_around_cluster()` - Find trigger primitives near cluster

## Parameter System

### ParametersManager
- **Location**: `src/lib/inc/ParametersManager.h`
- **Usage**: 
  ```cpp
  #include "ParametersManager.h"
  double value = GET_PARAM_DOUBLE("geometry.apa_length_cm");
  ```
- **Documentation**: [PARAMETERS.md](../parameters/PARAMETERS.md)

### Legacy Constants
- **Location**: `src/lib/inc/utils.h`
- **Description**: Backward-compatible access to parameters
- **Usage**: Direct constants like `apa_lenght_in_cm`, `wire_pitch_in_cm_collection`

## Utility Functions

### Global Utilities
- **Location**: `src/lib/inc/global.h`
- **Functions**: General utility functions used across the project

### Geometry Constants
- **Location**: `src/lib/inc/utils.h`
- **Namespaces**: `APA::` (detector configuration), `PDG::` (particle codes)

## Application Interfaces

### Main Applications
Located in `src/app/src/`:
- **cluster.cpp** - Main clustering application
- **backtrack.cpp** - Position backtracking application  
- **analyze_tps.cpp** - Trigger primitive analysis
- **analyze_clusters.cpp** - Cluster analysis

### Script Interfaces
Located in `scripts/`:
- **cluster.sh** - Clustering workflow script
- **backtrack.sh** - Backtracking workflow script
- **analyze_tps.sh** - TP analysis workflow script

See [scripts/README.md](../scripts/README.md) for detailed usage.

## Configuration Interfaces

### JSON Configuration
- **Location**: `json/` directory
- **Documentation**: [json/README.md](../json/README.md)
- **Usage**: Algorithm parameters, file paths, processing options

### Parameter Files
- **Location**: `parameters/` directory  
- **Format**: `.dat` files with `< key = value >` syntax
- **Documentation**: [parameters/PARAMETERS.md](../parameters/PARAMETERS.md)

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