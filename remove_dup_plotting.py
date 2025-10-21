# Remove duplicate plotting section from analyze_clusters.cpp
with open('src/app/analyze_clusters.cpp', 'r') as f:
    lines = f.readlines()

# Find the line numbers
new_section_start = -1
old_section_start = -1
old_section_end = -1

for i, line in enumerate(lines):
    if 'PLOTTING SECTION - Generate combined plots' in line:
        new_section_start = i
    elif new_section_start > 0 and old_section_start < 0 and 'if (!min_cluster_charge.empty())' in line:
        old_section_start = i - 1  # Include preceding line
    elif old_section_start > 0 and "Don't close PDF here - will be closed after all files are processed" in line:
        old_section_end = i + 4  # Include the f->Close(), delete f, comment, and closing brace

print(f"New plotting section starts at line {new_section_start + 1}")
print(f"Old duplicate section: lines {old_section_start + 1} to {old_section_end + 1}")

# Remove the old section
if old_section_start > 0 and old_section_end > old_section_start:
    new_lines = lines[:old_section_start] + lines[old_section_end + 1:]
    
    with open('src/app/analyze_clusters.cpp', 'w') as f:
        f.writelines(new_lines)
    
    print(f"Removed {old_section_end - old_section_start + 1} lines")
    print("File updated successfully!")
else:
    print("Could not identify sections to remove")
