#!/usr/bin/env python3
"""Quick script to inspect NPZ file metadata"""

import numpy as np
import sys
from pathlib import Path

if len(sys.argv) < 2:
    print("Usage: python3 inspect_npz.py <npz_file>")
    sys.exit(1)

npz_file = Path(sys.argv[1])
if not npz_file.exists():
    print(f"Error: File {npz_file} not found")
    sys.exit(1)

print(f"\n{'='*60}")
print(f"Inspecting: {npz_file.name}")
print(f"{'='*60}\n")

# Load the NPZ file
data = np.load(npz_file)

print("Arrays in file:")
for key in data.files:
    arr = data[key]
    print(f"  {key}: shape={arr.shape}, dtype={arr.dtype}")

print("\nMetadata structure (first 5 entries):")
print("Format: [event, is_marley, is_main_track, is_es_interaction,")
print("         true_vtx_x, true_vtx_y, true_vtx_z,")
print("         true_mom_x, true_mom_y, true_mom_z,")
print("         true_nu_energy, true_particle_energy, plane_id]")
print()

metadata = data['metadata']
n_samples = min(5, len(metadata))

for i in range(n_samples):
    m = metadata[i]
    print(f"Entry {i}:")
    print(f"  Event: {int(m[0])}")
    print(f"  Flags: is_marley={bool(m[1])}, is_main_track={bool(m[2])}")
    print(f"  Interaction type: is_es_interaction={bool(m[3])} (1=ES, 0=CC)")
    print(f"  True vertex: ({m[4]:.2f}, {m[5]:.2f}, {m[6]:.2f}) cm")
    print(f"  True momentum: ({m[7]:.4f}, {m[8]:.4f}, {m[9]:.4f}) GeV/c")
    print(f"  True energies: nu={m[10]:.4f} GeV, particle={m[11]:.4f} GeV")
    print(f"  Plane: {int(m[12])}")
    print()

# Statistics
print("\nStatistics over all entries:")
print(f"Total entries: {len(metadata)}")
print(f"Interaction type distribution:")
is_es = metadata[:, 3].astype(bool)
n_es = np.sum(is_es)
n_cc = np.sum(~is_es)
print(f"  ES interactions: {n_es} ({100*n_es/len(metadata):.1f}%)")
print(f"  CC interactions: {n_cc} ({100*n_cc/len(metadata):.1f}%)")

print(f"\nIs MARLEY: {np.sum(metadata[:, 1].astype(bool))} ({100*np.sum(metadata[:, 1].astype(bool))/len(metadata):.1f}%)")
print(f"Is main track: {np.sum(metadata[:, 2].astype(bool))} ({100*np.sum(metadata[:, 2].astype(bool))/len(metadata):.1f}%)")

# Check for problematic values
print(f"\nData quality checks:")
mom_mag = np.sqrt(metadata[:, 7]**2 + metadata[:, 8]**2 + metadata[:, 9]**2)
n_zero_mom = np.sum(mom_mag < 1e-6)
print(f"  Entries with zero momentum: {n_zero_mom} ({100*n_zero_mom/len(metadata):.1f}%)")

n_negative_energy = np.sum(metadata[:, 10] < 0)
print(f"  Entries with negative neutrino energy: {n_negative_energy} ({100*n_negative_energy/len(metadata):.1f}%)")

# Show some momentum magnitudes
print(f"\nMomentum magnitude statistics (GeV/c):")
print(f"  Min: {np.min(mom_mag):.4f}")
print(f"  Max: {np.max(mom_mag):.4f}")
print(f"  Mean: {np.mean(mom_mag):.4f}")
print(f"  Median: {np.median(mom_mag):.4f}")

print(f"\nNeutrino energy statistics (GeV):")
valid_nu_energies = metadata[metadata[:, 10] > 0, 10]
if len(valid_nu_energies) > 0:
    print(f"  Min: {np.min(valid_nu_energies):.4f}")
    print(f"  Max: {np.max(valid_nu_energies):.4f}")
    print(f"  Mean: {np.mean(valid_nu_energies):.4f}")
    print(f"  Median: {np.median(valid_nu_energies):.4f}")
else:
    print("  No valid neutrino energies found!")

print()
