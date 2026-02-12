# Metadata Structure Update

## Summary

Updated metadata format to use momentum variables instead of direction vectors, and reorganized field order to group interaction classification at the beginning.

## Changes

### New Metadata Structure (15 values)

```
[0]: is_marley (1.0 if Marley, 0.0 otherwise)
[1]: is_main_track (1.0 if main track, 0.0 otherwise)
[2]: is_es_interaction (1.0 for ES, 0.0 for CC/UNKNOWN) - MOVED HERE
[3-5]: true_pos (x, y, z) [cm]
[6-8]: true_particle_mom (px, py, pz) [GeV/c] - CHANGED from true_dir
[9-11]: true_nu_mom (px, py, pz) [GeV/c] - NEW
[12]: true_nu_energy [MeV]
[13]: true_particle_energy [MeV]
[14]: plane_id (0=U, 1=V, 2=X)
```

### Old Metadata Structure (15 values)

```
[0]: is_marley
[1]: is_main_track
[2-4]: true_pos (x, y, z)
[5-7]: true_dir (dx, dy, dz) - normalized direction
[8-10]: true_mom (px, py, pz) - particle momentum
[11]: true_nu_energy
[12]: true_particle_energy
[13]: plane_id
[14]: is_es_interaction - WAS AT END
```

## Key Changes

1. **is_es_interaction moved to position [2]**: Now grouped with other classification flags at the start
2. **Removed true_dir**: Direction vectors no longer included
3. **Renamed true_mom → true_particle_mom**: Clearer naming for particle momentum
4. **Added true_nu_mom**: New field for neutrino momentum (positions [9-11])
5. **Reordered fields**: Position, momenta, energies, then plane_id

## File Structure Changes

### online-pointing-utils

- **Moved**: `scripts/generate_cluster_arrays.py` → `python/generate_cluster_arrays.py`
- **Updated**: `scripts/generate_cluster_images.sh` now calls `../python/generate_cluster_arrays.py`
- **Policy**: Keep only bash scripts in `scripts/`, Python code goes in `python/`

### Updated Files

**online-pointing-utils:**
- `python/generate_cluster_arrays.py` (moved and updated)
  - Updated branches to read `true_nu_mom_x/y/z` instead of `true_dir_x/y/z`
  - Reordered metadata array construction
  - Updated comments and documentation

- `scripts/generate_cluster_images.sh`
  - Updated path to Python script

**ml_for_pointing:**
- `python/data_loader.py`
  - Updated `parse_metadata()` to handle new 15-value format
  - Updated all function docstrings (shape (N, 15))
  - Updated `get_dataset_statistics()` to read plane_id from index 14

## Migration Notes

- **Old NPZ files will be incompatible** with the new code
- Need to regenerate all cluster arrays with updated script
- The new format provides more physics information (neutrino momentum)
- ES interaction flag is now more accessible at the beginning of metadata array
