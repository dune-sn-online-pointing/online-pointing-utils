#!/usr/bin/env python3
"""Quick diagnostic to check ToT values in a ROOT file"""
import sys
import ROOT

if len(sys.argv) < 2:
    print("Usage: python3 check_tot_in_file.py <root_file>")
    sys.exit(1)

filename = sys.argv[1]
f = ROOT.TFile.Open(filename)
if not f or f.IsZombie():
    print(f"Error: Cannot open {filename}")
    sys.exit(1)

tree_path = "triggerAnaDumpTPs/TriggerPrimitives/tpmakerTPC__TriggerAnaTree1x2x2"
tree = f.Get(tree_path)
if not tree:
    print(f"Error: Tree not found at {tree_path}")
    sys.exit(1)

print(f"Checking file: {filename}")
print(f"Total entries in tree: {tree.GetEntries()}")

# Check first 100 entries or all if less
n_check = min(100, tree.GetEntries())
tot_values = []
for i in range(n_check):
    tree.GetEntry(i)
    tot = tree.samples_over_threshold if hasattr(tree, 'samples_over_threshold') else 0
    tot_values.append(tot)

nonzero = sum(1 for t in tot_values if t > 0)
print(f"Checked first {n_check} TPs:")
print(f"  Non-zero ToT: {nonzero}/{n_check} ({100*nonzero/n_check:.1f}%)")
print(f"  ToT range: [{min(tot_values)}, {max(tot_values)}]")

if nonzero == 0:
    print("\n⚠️  WARNING: All checked TPs have ToT=0")
    print("   This file may be from an older format or simulation without ToT info")
else:
    print(f"\n✓ File appears normal (has ToT data)")

f.Close()
