# Test Suite# Test Directory



Automated tests for the online-pointing-utils framework to ensure nothing breaks during development.This directory contains automated tests for the online-pointing-utils framework.



## Quick Start## Test Data



Run all tests:- `test_es_tpstream.root` - Small ES sample tpstream file for testing

```bash- `test_bg_tps.root` - Background TPs file for testing

./run_all_tests.sh

```## Running Tests



Clean previous outputs and run tests:To run all tests:

```bash```bash

./run_all_tests.sh --clean./run_tests.sh

``````



## Test StructureTo clean previous test outputs and run tests:

```bash

```./run_tests.sh --clean

test/```

├── data/                      # Test input files (committed to repo)

│   ├── test_cc_tpstream.root  # CC sample for testing## What Gets Tested

│   └── test_es_tpstream.root  # ES sample for testing

│The test suite runs through the complete analysis pipeline:

├── test_backtrack/            # Tests for backtrack_tpstream

│   ├── test_backtrack.sh1. **Backtracking** - Tests backtrack_tpstream app

│   ├── test_backtrack_es.json   - Input: tpstream file

│   └── test_backtrack_cc.json   - Output: backtracked TPs file

│

├── test_clustering/           # Tests for make_clusters2. **Add Backgrounds** - Tests add_backgrounds app

│   ├── test_clustering.sh   - Input: backtracked TPs + background TPs

│   ├── test_clustering_es.json   - Output: merged TPs file

│   └── test_clustering_cc.json

│3. **Clustering** - Tests make_clusters app

├── test_matching/             # Tests for match_clusters   - Input: merged TPs file

│   ├── test_matching.sh   - Output: cluster files

│   ├── test_matching_es.json

│   └── test_matching_cc.json4. **Analysis** - Tests analyze_clusters app

│   - Input: cluster files

├── test_analysis/             # Tests for analysis apps   - Output: analysis PDF and statistics

│   ├── test_analyze_clusters.sh

│   ├── test_analyze_tps.sh5. **Python Analysis** - Tests python/ana/ scripts

│   ├── test_analyze_matching.sh   - Input: cluster files

│   └── *.json (configs for each)   - Output: text analysis files

│

└── run_all_tests.sh           # Master test runner## Test Outputs

```

All test outputs are saved to `test_output/` directory:

## Individual Test Suites- Individual app logs: `test_output/<test_name>.log`

- Intermediate ROOT files

You can run individual test suites:- Analysis results



```bash## Adding New Tests

# Test backtracking

cd test_backtrack && ./test_backtrack.shTo add a new test:

1. Add test data files to this directory (keep them small!)

# Test clustering2. Create a JSON configuration in the test script

cd test_clustering && ./test_clustering.sh3. Add a `run_test` call in `run_tests.sh`

4. Update this README

# Test matching
cd test_matching && ./test_matching.sh

# Test analysis
cd test_analysis && ./test_analyze_clusters.sh
cd test_analysis && ./test_analyze_tps.sh
cd test_analysis && ./test_analyze_matching.sh
```

## Test Coverage

Each test suite tests both **ES** and **CC** samples:

### 1. Backtracking (`backtrack_tpstream`)
- Input: tpstream files
- Output: backtracked TPs files
- Tests: ES and CC samples

### 2. Clustering (`make_clusters`)
- Input: backtracked TPs files
- Output: cluster files
- Tests: ES and CC samples with standard parameters (tick=3, ch=2, min=2, tot=1)

### 3. Matching (`match_clusters`)
- Input: cluster files
- Output: matched multiplane clusters
- Tests: ES and CC samples with time_tolerance=10 ticks

### 4. Analysis Apps
- **`analyze_clusters`**: Generates PDF reports from cluster files
- **`analyze_tps`**: Generates PDF reports from TPs files
- **`analyze_matching`**: Analyzes matching metrics

All analysis apps tested on both ES and CC samples.

## Test Data

The test data files are small samples (< 3 MB each) committed to the repository:

- `test_es_tpstream.root`: Elastic scattering sample (254 KB)
- `test_cc_tpstream.root`: Charged current sample (2.1 MB)

These files are sufficient for regression testing and CI/CD pipelines.

## Output Files

Test outputs are generated in `output/` subdirectories within each test suite folder:
- `test_backtrack/output/` - Backtracked TPs
- `test_clustering/output/` - Cluster files
- `test_matching/output/` - Matched clusters
- `test_analysis/output/` - Analysis PDFs and reports

**Note:** Output directories are `.gitignore`d and should not be committed.

## Adding New Tests

To add a new test:

1. Create a new directory: `test/test_<app_name>/`
2. Add test script: `test_<app_name>.sh`
3. Add JSON configs: one for ES, one for CC
4. Add test to `run_all_tests.sh`

## CI/CD Integration

This test suite is designed to be run in CI/CD pipelines:

```bash
# In CI/CD pipeline
cd test/
./run_all_tests.sh
```

Exit codes:
- `0`: All tests passed
- `1`: One or more tests failed

## When to Run Tests

Run tests:
- After modifying any C++ application code
- After changing clustering/matching algorithms
- Before committing major changes
- Before merging pull requests
- When reorganizing code structure
