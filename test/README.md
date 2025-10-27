# Test Directory

This directory contains automated tests for the online-pointing-utils framework.

## Test Data

- `test_es_tpstream.root` - Small ES sample tpstream file for testing
- `test_bg_tps.root` - Background TPs file for testing

## Running Tests

To run all tests:
```bash
./run_tests.sh
```

To clean previous test outputs and run tests:
```bash
./run_tests.sh --clean
```

## What Gets Tested

The test suite runs through the complete analysis pipeline:

1. **Backtracking** - Tests backtrack_tpstream app
   - Input: tpstream file
   - Output: backtracked TPs file

2. **Add Backgrounds** - Tests add_backgrounds app
   - Input: backtracked TPs + background TPs
   - Output: merged TPs file

3. **Clustering** - Tests make_clusters app
   - Input: merged TPs file
   - Output: cluster files

4. **Analysis** - Tests analyze_clusters app
   - Input: cluster files
   - Output: analysis PDF and statistics

5. **Python Analysis** - Tests python/ana/ scripts
   - Input: cluster files
   - Output: text analysis files

## Test Outputs

All test outputs are saved to `test_output/` directory:
- Individual app logs: `test_output/<test_name>.log`
- Intermediate ROOT files
- Analysis results

## Adding New Tests

To add a new test:
1. Add test data files to this directory (keep them small!)
2. Create a JSON configuration in the test script
3. Add a `run_test` call in `run_tests.sh`
4. Update this README
