# Pentagon Algorithm Improvements

## Summary

Updated the pentagon calculation algorithm in `Display.cpp` to use a scanning approach similar to the improved Python version, with fallback to the original parametric method.

## Key Changes

### 1. Added Shoelace Formula for Polygon Area
```cpp
double polygonArea(const std::vector<std::pair<double, double>>& vertices)
```
- Calculates exact polygon area using the Shoelace formula
- More accurate than analytical approximations

### 2. Improved Pentagon Vertex Selection
The new algorithm:
- **Scans candidate vertices** (t1, t2) in a grid search
- **Tries 10x10 = 100 different configurations** for intermediate points
- **Minimizes area difference** between pentagon and target waveform integral
- **Falls back** to the original parametric approach if no good solution is found

### 3. Algorithm Flow

```
1. Calculate target_area from adc_integral + offset
2. Grid search over intermediate point positions:
   - t1 ranges from time_start to time_peak
   - t2 ranges from time_peak to time_end
3. For each (t1, t2) pair:
   - Calculate interpolated heights y1, y2
   - Build pentagon vertices
   - Calculate area using Shoelace formula
   - Track configuration with minimum area difference
4. If good match found: use it
5. Otherwise: fall back to parametric approach (frac=0.5)
```

### 4. Comparison with Original

| Aspect | Original (Parametric) | Improved (Scanning) |
|--------|----------------------|---------------------|
| Approach | Fixed fraction parameter (0.5) | Grid search over candidates |
| Flexibility | Limited to symmetric shapes | Adapts to asymmetric waveforms |
| Accuracy | Depends on recursive adjustment | Direct area minimization |
| Robustness | May fail on edge cases | Has parametric fallback |
| Speed | Fast (analytical) | Slower (100 iterations) but still fast |

## Testing

A Python test script has been created: `scripts/test_pentagon_algorithm.py`

### Run Tests
```bash
cd /home/virgolaema/dune/online-pointing-utils
python3 scripts/test_pentagon_algorithm.py
```

This will:
- Generate synthetic waveforms
- Compare old vs improved approaches
- Save comparison plots to `/tmp/pentagon_test_case_*.png`
- Print quantitative metrics (area differences, relative errors)

### Test Cases
The script includes 6 test cases:
1. Short TP (6 ticks): 1369→1375, peak=1372
2. Short TP (6 ticks): 1389→1395, peak=1392  
3. Medium TP (8 ticks): 1376→1384, peak=1380
4. Symmetric long TP: 100→120, peak=110
5. Asymmetric (early peak): 100→120, peak=105
6. Asymmetric (late peak): 100→120, peak=115

## Files Modified

- `src/ana/Display.cpp`: Added `polygonArea()` and improved `calculatePentagonParams()`
- `scripts/test_pentagon_algorithm.py`: New test/validation script

## Expected Impact

- **Better visual accuracy**: Pentagons should match waveform shapes more closely
- **Handles edge cases**: Asymmetric waveforms, short TPs, varying peak positions
- **Backward compatible**: Falls back to original method if scanning fails
- **Minimal performance impact**: ~100 iterations per TP, still very fast

## Notes

- The scanning approach tries 10 positions on each side (left and right of peak)
- This can be tuned by changing `n_samples` in the code
- The threshold is approximated as `offset / (time_end - time_start)`
- Intermediate heights use linear interpolation as starting guess
