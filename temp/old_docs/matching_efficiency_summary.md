# Matching Efficiency vs Time Window

Subset: five-file validation sample (225 main X clusters) using the `match_clusters` executable with the `json/cc_valid_tXXX.json` configs.

| Time window (ticks) | Complete (X+U+V) | Partial U | Partial V | Total matched | Unmatched | Avg truth ΔU (cm) | Avg truth ΔV (cm) |
|---------------------|------------------|-----------|-----------|---------------|-----------|-------------------|-------------------|
| 500                 | 220 (97.8%)      | 4 (1.8%)  | 0 (0.0%)  | 224 (99.6%)   | 1 (0.4%)  | 47.6              | 48.2              |
| 300                 | 220 (97.8%)      | 4 (1.8%)  | 0 (0.0%)  | 224 (99.6%)   | 1 (0.4%)  | 47.6              | 48.2              |
| 100                 | 219 (97.3%)      | 4 (1.8%)  | 0 (0.0%)  | 223 (99.1%)   | 2 (0.9%)  | 35.5              | 37.5              |
| 50                  | 217 (96.4%)      | 5 (2.2%)  | 0 (0.0%)  | 222 (98.7%)   | 3 (1.3%)  | 20.1              | 20.8              |
| 40                  | 216 (96.0%)      | 5 (2.2%)  | 1 (0.4%)  | 222 (98.7%)   | 3 (1.3%)  | 14.8              | 13.4              |
| 30                  | 216 (96.0%)      | 5 (2.2%)  | 0 (0.0%)  | 221 (98.2%)   | 4 (1.8%)  | 2.8               | 5.9               |
| 20                  | 215 (95.6%)      | 5 (2.2%)  | 0 (0.0%)  | 220 (97.8%)   | 5 (2.2%)  | 1.2               | 1.2               |
| 10                  | 215 (95.6%)      | 5 (2.2%)  | 0 (0.0%)  | 220 (97.8%)   | 5 (2.2%)  | 0.0               | 0.0               |
| 5                   | 192 (85.3%)      | 4 (1.8%)  | 21 (9.3%) | 217 (96.4%)   | 8 (3.6%)  | 0.0               | 0.0               |
| 0                   | 158 (70.2%)      | 10 (4.4%) | 33 (14.7%) | 201 (89.3%)   | 24 (10.7%) | 0.0               | 2.4               |

Residuals were obtained by re-running `match_clusters` for each tolerance (five-file slice, `-m 5`) with `scripts/compute_truth_distances.py` and measuring 3D truth separations between the matched U/V clusters and the associated X cluster. Broad time windows occasionally grab wrong-plane partners (≈48 cm bias), while ≤20 ticks drives the residuals below ~1 cm; at 10 ticks both planes align exactly on truth for the processed events.

## Truth cross-check at 500 ticks

`match_clusters_truth` run with `json/cc_valid_t500.json` confirmed:

- Main X clusters with truth: 225 (none missing)
- Truth-matched complete: 216
- Truth-matched U-only: 5
- Truth-matched V-only: 1
- Unmatched despite truth: 3 (closest candidates ~310 cm away)
- Mean truth residuals: 0 cm for both U and V matches

### Matching behavior notes

- The batch driver records the **first** U and V candidates that satisfy time, event, and APA checks for each X cluster (`x_to_u_match` / `x_to_v_match` in `src/app/match_clusters.cpp`). Additional compatible candidates are scanned but ignored, so an X cluster never contributes more than one U or V partner.
- Once a pair passes these filters, spatial compatibility (`are_compatibles`) is the final arbiter for forming a complete match. Failing the spatial check drops the combination entirely, leaving the X cluster in a partial bucket if only one view was kept.
- When the time window shrinks below ~10 ticks, most losses come from event-offset failures: the histograms in the verbose logs show deltas of ±1, ±5–13 ticks dominating the rejects.

Source logs: `run_t500.log`, `run_t300.log`, `run_t100.log`, `run_t50.log`, `run_t40.log`, `run_t30.log`, `run_t20.log`, `run_t10.log`, `run_t5.log`, `run_t0.log`, `run_truth_t500.log`.
