# Python-C++ Integration: Design Decision

## Problem
The `generate_cluster_arrays.py` script needs to determine the clusters folder path using the same logic as the C++ code in `src/lib/utils.cpp::getClustersFolder()`.

## Options Considered

### Option 1: pybind11 Bindings ❌ (Not Chosen)

**What is pybind11?**
pybind11 is a lightweight C++11 library that creates Python bindings for C++ code. It allows Python to directly call C++ functions.

**How it works:**
1. Create a binding module (e.g., `utils_bindings.cpp`):
```cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  // For std::string, std::vector
#include "utils.hpp"

namespace py = pybind11;

PYBIND11_MODULE(utils_cpp, m) {
    m.doc() = "Python bindings for utils.cpp";
    
    m.def("get_clusters_folder", 
          &getClustersFolder,
          py::arg("json_config"),
          "Get clusters folder path from JSON config");
}
```

2. Update CMakeLists.txt:
```cmake
find_package(pybind11 REQUIRED)
pybind11_add_module(utils_cpp src/bindings/utils_bindings.cpp)
target_link_libraries(utils_cpp PRIVATE clustersLibs)
```

3. Use in Python:
```python
import utils_cpp
import json

with open('config.json') as f:
    config = json.load(f)

folder = utils_cpp.get_clusters_folder(config)
```

**Advantages:**
- ✅ Single source of truth (C++ code)
- ✅ Type-safe
- ✅ Fast (native C++ performance)
- ✅ Automatic type conversions

**Disadvantages:**
- ❌ Requires pybind11 dependency
- ❌ Adds build complexity
- ❌ Users must compile Python bindings
- ❌ Cross-platform build issues
- ❌ Version compatibility maintenance
- ❌ Overkill for simple string formatting

### Option 2: Pure Python Implementation ✅ (Chosen)

**Implementation:**
Reimplement `getClustersFolder()` logic in Python directly in `generate_cluster_arrays.py`.

**Advantages:**
- ✅ No compilation needed
- ✅ No additional dependencies
- ✅ Works immediately after git clone
- ✅ Easy to debug
- ✅ Cross-platform compatible
- ✅ Self-contained script

**Disadvantages:**
- ❌ Code duplication (but minimal)
- ❌ Must keep Python/C++ in sync

### Option 3: Subprocess Call ❌ (Not Practical)

Call a C++ executable that outputs the folder path.

**Disadvantages:**
- ❌ Would need to create dedicated executable
- ❌ Subprocess overhead
- ❌ Awkward interface

## Decision: Pure Python Implementation

**Rationale:**
1. **Simplicity:** The `getClustersFolder()` function is straightforward string formatting—no complex algorithms or C++ features that benefit from native implementation.

2. **Portability:** Python scripts should be runnable without compilation. Users can `git clone` and immediately run the script.

3. **Maintenance:** The logic is stable and unlikely to change frequently. If it does, updating both is trivial.

4. **Verification:** Created `test_clusters_folder.py` to verify Python matches C++ output exactly.

## Implementation Details

### C++ Version (src/lib/utils.cpp)
```cpp
std::string getClustersFolder(const nlohmann::json& j) {
    std::string cluster_prefix = j.value("clusters_folder_prefix", "clusters");
    int tick_limit = j.value("tick_limit", 0);
    int channel_limit = j.value("channel_limit", 0);
    int min_tps_to_cluster = j.value("min_tps_to_cluster", 0);
    int tot_cut = j.value("tot_cut", 0);
    float energy_cut = j.value("energy_cut", 0.0f);
    std::string outfolder = j.value("clusters_folder", ".");
    
    // Build folder name with sanitization...
    return outfolder + "/" + clusters_subfolder;
}
```

### Python Version (scripts/generate_cluster_arrays.py)
```python
def get_clusters_folder(json_config):
    """Matches src/lib/utils.cpp::getClustersFolder()"""
    cluster_prefix = j.get("clusters_folder_prefix", "clusters")
    tick_limit = j.get("tick_limit", 0)
    channel_limit = j.get("channel_limit", 0)
    min_tps_to_cluster = j.get("min_tps_to_cluster", 0)
    tot_cut = j.get("tot_cut", 0)
    energy_cut = float(j.get("energy_cut", 0.0))
    outfolder = j.get("clusters_folder", ".").rstrip('/')
    
    # Build folder name with sanitization...
    return f"{outfolder}/{clusters_subfolder}"
```

### Verification
Test suite in `scripts/test_clusters_folder.py` ensures Python implementation produces identical output to C++ for various configurations.

## Usage

### With JSON config (recommended):
```bash
# Bash script
./scripts/generate_cluster_images.sh --json json/es_valid.json

# Python directly
python3 scripts/generate_cluster_arrays.py --json json/es_valid.json
```

### Direct folder specification (legacy):
```bash
./scripts/generate_cluster_images.sh data/prod_es/clusters_es_valid_bg_tick3_ch2_min2_tot1_e0
```

## When Would pybind11 Be Appropriate?

Use pybind11 when:
- Complex algorithms that benefit from C++ performance
- Heavy numerical computation
- Need to pass complex C++ objects (not just primitives)
- Existing large C++ codebase to expose
- Performance-critical code path

**Not for:**
- Simple string formatting (this case)
- One-off utility functions
- Scripts that should be standalone/portable

## References
- pybind11 documentation: https://pybind11.readthedocs.io/
- C++ implementation: `src/lib/utils.cpp::getClustersFolder()`
- Python implementation: `scripts/generate_cluster_arrays.py::get_clusters_folder()`
- Test suite: `scripts/test_clusters_folder.py`
