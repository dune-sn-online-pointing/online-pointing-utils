# Test Suite

This directory contains regression tests for the C++ apps in this branch.

## Quick Start

Run all tests:

```bash
./run_all_tests.sh
```

Clean previous outputs and run tests:

```bash
./run_all_tests.sh --clean
```

## Test Data

Small tpstream ROOT samples committed to the repo:
- test/data/test_es_tpstream.root
- test/data/test_cc_tpstream.root

Helper lists for backtracking:
- test/data/test_es_tpstreams.txt
- test/data/test_cc_tpstreams.txt

## What Gets Tested

1. Backtracking (backtrack)
   - Inputs: tpstream file lists
   - Outputs: *_tps_bktr0.root files in test/test_backtrack/output

2. Clustering (cluster)
   - Inputs: backtracked TPs
   - Outputs: *_clusters_tick*_tot*.root files in test/test_clustering/output

3. Matching (match_clusters)
   - Currently disabled on this branch (app not ported)

4. Analysis
   - analyze_tps produces PDFs in test/test_analysis/output
   - analyze_clusters produces PDFs in test/test_analysis/output

## Outputs

Outputs are written to output/ subfolders and are gitignored:
- test/test_backtrack/output
- test/test_clustering/output
- test/test_matching/output
- test/test_analysis/output

## Adding a Test

1. Create a test_<app>/ folder with a test_<app>.sh script.
2. Add JSON configs (one for ES, one for CC).
3. Add the test to run_all_tests.sh.
