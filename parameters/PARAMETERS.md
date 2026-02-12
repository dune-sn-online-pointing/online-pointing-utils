# Parameter System Documentation

## Overview

The parameter system uses `.dat` files with a structured format instead of hardcoded C++ constants. This allows for easier configuration management and runtime parameter modification.

## Parameter File Format

Parameters are stored in `.dat` files with the following format:

```
*******************************************************************************
Section Name
*******************************************************************************

< parameter.name = value >    Description of parameter
```

### Example:
```
< geometry.apa_length_cm = 230.0 >    APA length in cm
< timing.time_tick_cm = 0.0805 >      Time tick in cm
```

## Environment Setup

Set the `PARAMETERS_DIR` environment variable to point to the parameters directory:

```bash
export PARAMETERS_DIR=/path/to/online-pointing-utils/parameters
```

Or use the provided setup script:
```bash
source setup_parameters.sh
```

The `init.sh` script automatically sets `PARAMETERS_DIR` when initializing the environment.

## Usage in Code

### Include the header:
```cpp
#include "ParametersManager.h"
#include "utils.h"  // For legacy compatibility
```

### Access parameters:
```cpp
// Direct access via ParametersManager
double apa_length = GET_PARAM_DOUBLE("geometry.apa_length_cm");
int time_window = GET_PARAM_INT("timing.time_window");
std::string detector_name = GET_PARAM_STRING("detector.name");

// Legacy compatibility macros (deprecated)
double length = apa_lenght_in_cm;  // Uses GET_PARAM_DOUBLE internally
```

### Manual parameter loading (if needed):
```cpp
ParametersManager::getInstance().loadParameters();
```

## Parameter Files

### geometry.dat
- APA dimensions and wire configurations
- Detector geometry constants

### timing.dat  
- Time-related constants (ticks, drift speed, etc.)
- Clock and conversion factors

### conversion.dat
- Unit conversion factors (ADC to energy, etc.)

### detector.dat
- Detector-specific settings
- Number of APAs, detector name, etc.

### analysis.dat
- Analysis-specific parameters
- Algorithm tuning parameters


## Derived Parameters

The system automatically calculates derived parameters:
- `geometry.wire_pitch_induction_cm` from diagonal pitch and angle
- `geometry.apa_angular_coeff` from APA angle
- `timing.tpc_sample_length_ns` from clock tick and conversion factor

## Error Handling

The system provides graceful error handling:
- Missing parameter files produce warnings but don't crash
- Missing parameters throw runtime exceptions with descriptive messages
- Use `hasParameter(key)` to check parameter existence before access