#!/usr/bin/env python3
"""
Interactive Volume Image Viewer

Navigate through volume images (.npz) with Previous/Next buttons.
Shows the pentagon channel array with detailed metadata.

Usage:
    python view_volumes_interactive.py <volume_folder_or_pattern> [--no-tps]
    
Options:
    --no-tps    Show clusters as colored dots instead of full TP images
                Colors: Red=Main Track, Orange=MARLEY, Blue=Background
    
Examples:
    # View all volumes in a folder (full TP mode)
    python view_volumes_interactive.py data2/prod_es/volume_images_*/
    
    # View with cluster dots mode
    python view_volumes_interactive.py data2/prod_es/volume_images_*/ --no-tps
    
    # View specific pattern
    python view_volumes_interactive.py "data2/prod_es/volume_images_*/*volume*.npz"
"""

import matplotlib.pyplot as plt
from matplotlib.widgets import Button
import numpy as np
from pathlib import Path
import argparse
import uproot
import glob
import sys

# Check if uproot is available
HAS_UPROOT = True

# Conversion factor from TDC counts to TPC ticks
TDC_TO_TPC_CONVERSION = 32


class VolumeViewer:
    def __init__(self, volume_files, show_tps=True):
        """Initialize the volume viewer with navigation controls"""
        self.volume_files = volume_files
        self.n_volumes = len(volume_files)
        self.show_tps = show_tps
        self.current_idx = 0
        
        # Create figure with three panels
        if show_tps:
            self.fig, (self.ax_tps, self.ax_clusters, self.ax_info) = plt.subplots(
                1, 3, figsize=(20, 8)
            )
        else:
            self.fig, (self.ax_clusters, self.ax_info) = plt.subplots(
                1, 2, figsize=(16, 8)
            )
            self.ax_tps = None
        
        # Navigation buttons
        self.ax_prev = plt.axes([0.35, 0.02, 0.1, 0.04])
        self.ax_next = plt.axes([0.55, 0.02, 0.1, 0.04])
        self.btn_prev = Button(self.ax_prev, 'Previous')
        self.btn_next = Button(self.ax_next, 'Next')
        
        self.btn_prev.on_clicked(self.prev_volume)
        self.btn_next.on_clicked(self.next_volume)
        
        # Keyboard shortcuts
        self.fig.canvas.mpl_connect('key_press_event', self.on_key)
        
        # Display first volume
        self.display_current_volume()
        
        plt.tight_layout(rect=[0, 0.06, 1, 0.96], pad=1.5)
        plt.show()
    
    def load_volume(self, npz_path):
        """Load volume data from .npz file"""
        data = np.load(npz_path, allow_pickle=True)
        
        # Handle different key names
        if 'pentagon_array' in data:
            image = data['pentagon_array']
        elif 'image' in data:
            image = data['image']
        else:
            # Try first array
            image = data[data.files[0]]
        
        # Extract metadata
        metadata = {}
        if 'metadata' in data:
            meta_item = data['metadata']
            if isinstance(meta_item, np.ndarray):
                if meta_item.shape == ():
                    # Scalar metadata
                    metadata = meta_item.item()
                elif meta_item.shape == (1,):
                    # Single-element array - extract the dict inside
                    metadata = meta_item[0]
                    if isinstance(metadata, dict):
                        pass  # Already a dict
                    elif hasattr(metadata, 'item'):
                        metadata = metadata.item()
                else:
                    metadata = {'raw': meta_item}
            else:
                metadata = meta_item
        
        # Try to get other metadata fields
        for key in data.files:
            if key not in ['pentagon_array', 'image', 'metadata']:
                val = data[key]
                if val.shape == ():
                    metadata[key] = val.item()
                else:
                    metadata[key] = val
        
        # Debug: Check if event is in metadata
        if 'event' not in metadata:
            print(f"Warning: 'event' not in metadata. Keys: {list(metadata.keys())[:10]}")
        
        return image, metadata
    
    def load_clusters_from_root(self, npz_path, metadata):
        """Load clusters from ROOT file and calculate their center positions"""
        if not HAS_UPROOT:
            print("Warning: uproot not available, cannot load clusters")
            return []
        
        # Get the plane from metadata
        plane = metadata.get('plane', 'X')
        main_cluster_id = metadata.get('main_cluster_id', -1)
        
        print(f"DEBUG: Looking for main cluster ID {main_cluster_id} (type: {type(main_cluster_id)})")
        
        # Construct ROOT file path from npz filename
        npz_name = Path(npz_path).stem  # e.g., "..._bg_bg_bg_clusters_planeX_volume000"
        
        # Remove "_planeX_volumeXXX" suffix to get base name
        base_parts = npz_name.split('_')
        # Find where "plane" appears and remove from there
        try:
            plane_idx = next(i for i, part in enumerate(base_parts) if part.startswith('plane'))
            base_name = '_'.join(base_parts[:plane_idx])
        except StopIteration:
            print(f"Warning: Could not parse plane from filename: {npz_name}")
            return []
        
        # Build a list of candidate ROOT filenames (handles varying _bg suffix counts)
        candidates = []
        if base_name:
            candidates.append(base_name)
            if base_name.endswith('_clusters'):
                prefix = base_name[:-len('_clusters')]
                # Try adding up to three additional _bg suffixes in case of naming differences
                for extra_bg in range(1, 4):
                    candidate = f"{prefix}_" + "_".join(['bg'] * extra_bg) + "_clusters"
                    if candidate not in candidates:
                        candidates.append(candidate)

        # Search for the ROOT file in the volume folder's parent and any sibling clusters folders
        volume_root = Path(npz_path).parent.parent
        search_dirs = [d for d in volume_root.glob('clusters*') if d.is_dir()]
        search_dirs.append(volume_root)

        root_file = None
        attempted_paths = []
        for directory in search_dirs:
            for candidate in candidates:
                candidate_path = directory / f"{candidate}.root"
                attempted_paths.append(candidate_path)
                if candidate_path.exists():
                    root_file = candidate_path
                    break
            if root_file is not None:
                break

        if root_file is None:
            print(f"Warning: ROOT file not found for volume {npz_name}")
            for path in attempted_paths:
                print(f"  tried: {path}")
            return []
        
        try:
            # Open ROOT file and gather cluster trees (kept + discarded)
            f = uproot.open(root_file)
            tree_directories = ['clusters', 'discarded']
            trees = []
            for directory in tree_directories:
                tree_name = f'{directory}/clusters_tree_{plane}'
                if tree_name not in f:
                    continue
                trees.append((directory, f[tree_name]))

            if not trees:
                print(f"Warning: No cluster trees found for plane {plane} in {root_file.name}")
                return []

            # Get center channel and time from metadata to define volume bounds
            center_ch = metadata.get('center_channel', 0)
            center_time = metadata.get('center_time_tpc', 0)
            image_shape = metadata.get('image_shape', (208, 1242))
            
            # Calculate volume bounds - the image represents a 100cm x 100cm region
            # Image coordinates map to detector coordinates around the center
            # We need to filter clusters to only those in this region
            volume_half_ch = image_shape[0] / 2  # Half width in image pixels
            volume_half_time = image_shape[1] / 2  # Half width in image pixels
            
            # Detector coordinate bounds
            ch_min = center_ch - volume_half_ch
            ch_max = center_ch + volume_half_ch
            time_min = center_time - volume_half_time
            time_max = center_time + volume_half_time
            
            print(f"  Volume bounds: ch=[{ch_min:.1f}, {ch_max:.1f}], time=[{time_min:.1f}, {time_max:.1f}]")
            
            clusters = []
            clusters_filtered_out = 0

            total_clusters_loaded = 0

            for directory, tree in trees:
                # Load cluster data (these are jagged arrays - one entry per cluster)
                cluster_ids = tree['cluster_id'].array(library='np')
                events = tree['event'].array(library='np')
                tp_channels = tree['tp_detector_channel'].array(library='np')
                tp_times_tdc = tree['tp_time_start'].array(library='np')
                marley_fractions = tree['marley_tp_fraction'].array(library='np')
                true_labels = tree['true_label'].array(library='np') if 'true_label' in tree.keys() else None
                main_flags = tree['is_main_cluster'].array(library='np') if 'is_main_cluster' in tree.keys() else None

                total_clusters_loaded += len(cluster_ids)

                for i in range(len(cluster_ids)):
                    # Each entry is a cluster with its TPs
                    cid = cluster_ids[i]
                    evt = events[i]
                    ch_vals = tp_channels[i]
                    time_vals_tdc = tp_times_tdc[i]
                    marley_frac = marley_fractions[i]
                    true_label = true_labels[i] if true_labels is not None else None
                    main_flag = main_flags[i] if main_flags is not None else False
                    
                    # Handle different data structures
                    if hasattr(cid, '__len__') and len(cid) > 0:
                        cid = cid[0]  # Take first value if array
                    if hasattr(evt, '__len__') and len(evt) > 0:
                        evt = evt[0]
                    if hasattr(marley_frac, '__len__') and len(marley_frac) > 0:
                        marley_frac = marley_frac[0]  # Take first value if array
                    if true_label is not None and hasattr(true_label, '__len__') and not isinstance(true_label, (str, bytes)) and len(true_label) > 0:
                        true_label = true_label[0]
                    if isinstance(true_label, bytes):
                        true_label = true_label.decode('utf-8')
                    if hasattr(main_flag, '__len__') and len(main_flag) > 0:
                        main_flag = main_flag[0]
                        
                    if len(ch_vals) == 0:
                        continue
                    
                    # Only keep clusters from the same event as the volume
                    if metadata.get('event') is not None and int(evt) != int(metadata['event']):
                        continue

                    # Convert times from TDC counts to TPC ticks to match volume metadata
                    time_vals = time_vals_tdc / TDC_TO_TPC_CONVERSION

                    # Calculate cluster center by averaging channel and time (in detector coordinates)
                    center_channel = np.mean(ch_vals)
                    center_time = np.mean(time_vals)
                    
                    # Filter by cluster center to match volume construction logic
                    if not (ch_min <= center_channel <= ch_max and
                            time_min <= center_time <= time_max):
                        clusters_filtered_out += 1
                        continue
                    
                    # Use absolute detector coordinates (not image coordinates)
                    abs_ch = center_channel
                    abs_time = center_time
                    
                    # Determine if MARLEY cluster (marley_tp_fraction > 0)
                    label_str = str(true_label) if true_label is not None else ''
                    is_marley = (float(marley_frac) > 0) or ('marley' in label_str.lower())
                    is_main_track = bool(main_flag)
                    is_discarded = (directory == 'discarded')
                    
                    # Debug: print if we find main track
                    if is_main_track:
                        print(f"Found main track! ID={cid}, channel={abs_ch:.1f}, time={abs_time:.1f}, MARLEY={is_marley}")
                    
                    # Set color and marker based on cluster type
                    if is_main_track:
                        color = '#FF0000'  # Red for main track
                        size = 400
                        marker = '*'
                        label = 'Main Track'
                        zorder = 10
                    elif is_discarded:
                        color = '#666666'  # Gray for discarded clusters
                        size = 80
                        marker = 'x'
                        label = 'Discarded'
                        zorder = 4
                    elif is_marley:
                        color = '#FF8800'  # Orange for MARLEY
                        size = 150
                        marker = 'o'
                        label = 'MARLEY'
                        zorder = 5
                    else:
                        color = '#0088FF'  # Blue for background
                        size = 80
                        marker = 'o'
                        label = 'Background'
                        zorder = 3
                    
                    clusters.append({
                        'cluster_id': int(cid),
                        'event': int(evt),
                        'channel': abs_ch,
                        'time': abs_time,
                        'time_tpc': time_vals,
                        'n_tps': len(ch_vals),
                        'is_marley': is_marley,
                        'is_main': is_main_track,
                        'is_discarded': is_discarded,
                        'source': directory,
                        'color': color,
                        'size': size,
                        'marker': marker,
                        'label': label,
                        'zorder': zorder
                    })
            
            print(f"  Total clusters (all dirs): {total_clusters_loaded}")
            print(f"  Clusters kept: {len(clusters)}, filtered out: {clusters_filtered_out}")
            
            n_main = sum(1 for c in clusters if c['is_main'])
            n_marley = sum(1 for c in clusters if c['is_marley'])
            n_bg = sum(1 for c in clusters if not c['is_marley'])
            n_discarded = sum(1 for c in clusters if c.get('is_discarded'))
            print(f"  Breakdown: Main={n_main}, MARLEY={n_marley}, Background={n_bg}, Discarded={n_discarded}")
            
            return clusters
            
        except Exception as e:
            print(f"Error loading clusters from ROOT: {e}")
            import traceback
            traceback.print_exc()
            return []
    
    def display_current_volume(self):
        """Display the current volume in side-by-side views"""
        npz_path = self.volume_files[self.current_idx]
        
        try:
            image, metadata = self.load_volume(npz_path)
        except Exception as e:
            print(f"Error loading {npz_path}: {e}")
            return
        
        # Clear previous plots
        self.ax_tps.clear()
        self.ax_clusters.clear()
        self.ax_info.clear()
        self.ax_info.axis('off')
        
        # Get detector coordinate bounds for the volume
        center_ch = metadata.get('center_channel', 0)
        center_time = metadata.get('center_time_tpc', 0)
        half_ch = image.shape[0] / 2
        half_time = image.shape[1] / 2
        
        # Detector coordinate extent for the image
        extent = [center_time - half_time, center_time + half_time,  # time range (x-axis)
                  center_ch - half_ch, center_ch + half_ch]  # channel range (y-axis)
        
        # LEFT PANEL: Full TP image with detector coordinates
        im = self.ax_tps.imshow(image, cmap='viridis', aspect='auto',
                                interpolation='nearest', origin='lower',
                                extent=extent)
        
        # Add colorbar if not present
        if not hasattr(self, 'colorbar'):
            self.colorbar = plt.colorbar(im, ax=self.ax_tps, label='Amplitude', fraction=0.046)
        else:
            self.colorbar.update_normal(im)
        
        self.ax_tps.set_xlabel('Time bin', fontsize=10, fontweight='bold')
        self.ax_tps.set_ylabel('Channel', fontsize=10, fontweight='bold')
        self.ax_tps.set_title('TPs (Full Image)', fontsize=11, fontweight='bold')
        self.ax_tps.grid(True, alpha=0.2, linestyle='--')
        
        # MIDDLE PANEL: Cluster dots
        clusters = self.load_clusters_from_root(npz_path, metadata)
        main_clusters = []
        marley_clusters = []
        bg_clusters = []
        
        if clusters:
            # Separate clusters by type for proper legend handling
            main_clusters = [c for c in clusters if c['is_main']]
            marley_clusters = [c for c in clusters if c['is_marley'] and not c['is_main'] and not c.get('is_discarded')]
            bg_clusters = [c for c in clusters if not c['is_marley'] and not c.get('is_discarded')]
            discarded_clusters = [c for c in clusters if c.get('is_discarded')]

            if bg_clusters:
                times = [c['time'] for c in bg_clusters]
                channels = [c['channel'] for c in bg_clusters]
                self.ax_clusters.scatter(times, channels, s=60, c='#0088FF',
                                         alpha=0.7, edgecolors='black', linewidths=1,
                                         label='Background', zorder=3, marker='o')

            if discarded_clusters:
                times = [c['time'] for c in discarded_clusters]
                channels = [c['channel'] for c in discarded_clusters]
                self.ax_clusters.scatter(times, channels, s=80, c='#666666',
                                         alpha=0.9, edgecolors='black', linewidths=1.5,
                                         label='Discarded', zorder=4, marker='x')

            if marley_clusters:
                times = [c['time'] for c in marley_clusters]
                channels = [c['channel'] for c in marley_clusters]
                self.ax_clusters.scatter(times, channels, s=120, c='#FF8800',
                                         alpha=0.8, edgecolors='black', linewidths=1.5,
                                         label='MARLEY', zorder=5, marker='o')

            if main_clusters:
                times = [c['time'] for c in main_clusters]
                channels = [c['channel'] for c in main_clusters]
                self.ax_clusters.scatter(times, channels, s=300, c='#FF0000',
                                         alpha=0.9, edgecolors='black', linewidths=2,
                                         label='Main Track', zorder=10, marker='*')

            self.ax_clusters.legend(loc='upper right', fontsize=9, framealpha=0.9)

            n_total_displayed = len(clusters)
            n_marley_displayed = len([c for c in clusters if c['is_marley']])
            n_bg_displayed = len(bg_clusters)
            n_discarded_displayed = len(discarded_clusters)
            n_main_displayed = len(main_clusters)
            count_text = (
                f'Total: {n_total_displayed}\n'
                f'MARLEY: {n_marley_displayed}\n'
                f'BG: {n_bg_displayed}\n'
                f'Discarded: {n_discarded_displayed}\n'
                f'Main: {n_main_displayed}'
            )
            self.ax_clusters.text(0.02, 0.98, count_text, transform=self.ax_clusters.transAxes,
                                  fontsize=9, verticalalignment='top',
                                  bbox=dict(boxstyle='round', facecolor='white', alpha=0.85))
        else:
            self.ax_clusters.text(0.5, 0.5, 'No clusters found',
                                  transform=self.ax_clusters.transAxes, ha='center', va='center',
                                  fontsize=12, color='gray')
        
        self.ax_clusters.set_xlabel('Time bin', fontsize=10, fontweight='bold')
        self.ax_clusters.set_ylabel('Channel', fontsize=10, fontweight='bold')
        self.ax_clusters.set_title('Clusters (Centers)', fontsize=11, fontweight='bold')
        self.ax_clusters.grid(True, alpha=0.2, linestyle='--')
        
        # Set axis limits to match the detector coordinate extent
        self.ax_clusters.set_xlim(extent[0], extent[1])  # time range
        self.ax_clusters.set_ylim(extent[2], extent[3])  # channel range
        
        # Extract MARLEY cluster information from metadata
        filename = npz_path.name
        
        # Build information panel using ASCII-only text
        info_lines = []
        info_lines.append(f"Volume {self.current_idx + 1}/{self.n_volumes}")
        info_lines.append("")
        
        display_name = filename
        if len(display_name) > 80:
            display_name = display_name[:37] + "..." + display_name[-37:]
        info_lines.append("File")
        info_lines.append(f"  {display_name}")
        info_lines.append("")
        
        info_lines.append("Image")
        info_lines.append(f"  Shape: {image.shape[0]} channels x {image.shape[1]} time bins")
        info_lines.append(f"  Size:  {image.shape[0] * image.shape[1]:,} pixels")
        info_lines.append("")
        
        info_lines.append("Statistics")
        info_lines.append(f"  Min:       {image.min():>8.2f}")
        info_lines.append(f"  Max:       {image.max():>8.2f}")
        info_lines.append(f"  Mean:      {image.mean():>8.2f}")
        info_lines.append(f"  Std Dev:   {image.std():>8.2f}")
        info_lines.append(f"  Non-zero:  {(image > 0).sum():>8,} pixels")
        sparsity = (image == 0).sum() / image.size * 100
        info_lines.append(f"  Sparsity:  {sparsity:>8.1f}%")
        info_lines.append("")
        
        if isinstance(metadata, dict) and metadata:
            event = metadata.get('event', '?')
            plane = metadata.get('plane', '?')
            main_cluster_id = metadata.get('main_cluster_id', '?')
            info_lines.append("Main Track")
            info_lines.append(f"  Event:     {event}")
            info_lines.append(f"  Plane:     {plane}")
            actual_main_ids = [c['cluster_id'] for c in main_clusters] if clusters else []
            if actual_main_ids:
                info_lines.append(f"  Metadata ID: {main_cluster_id}")
                if len(actual_main_ids) == 1:
                    info_lines.append(f"  ROOT ID:     {actual_main_ids[0]}")
                else:
                    info_lines.append(f"  ROOT IDs:    {actual_main_ids}")
            else:
                info_lines.append(f"  Metadata ID: {main_cluster_id}")
            
            center_ch = metadata.get('center_channel', None)
            center_time = metadata.get('center_time_tpc', None)
            vol_size = metadata.get('volume_size_cm', 100.0)
            if center_ch is not None and center_time is not None:
                info_lines.append("  Center:")
                info_lines.append(f"    Channel:    {center_ch:>8.1f}")
                info_lines.append(f"    Time (TPC): {center_time:>8.1f} ticks")
                info_lines.append(f"    Size:       {vol_size:>8.1f} cm")
            
            n_total = metadata.get('n_clusters_in_volume', '?')
            n_marley = metadata.get('n_marley_clusters', '?')
            n_bg = metadata.get('n_non_marley_clusters', '?')
            info_lines.append("")
            info_lines.append("Metadata Cluster Counts")
            info_lines.append(f"  Total:      {n_total}")
            info_lines.append(f"  MARLEY:     {n_marley}")
            info_lines.append(f"  Background: {n_bg}")
            
            if clusters:
                n_marley_displayed = len([c for c in clusters if c['is_marley']])
                n_bg_displayed = len([c for c in clusters if not c['is_marley']])
                n_main_displayed = len([c for c in clusters if c['is_main']])
                info_lines.append("")
                info_lines.append("Displayed Cluster Counts")
                info_lines.append(f"  Total:      {len(clusters)}")
                info_lines.append(f"  MARLEY:     {n_marley_displayed}")
                info_lines.append(f"  Background: {n_bg_displayed}")
                info_lines.append(f"  Main:       {n_main_displayed}")
            
            interaction = metadata.get('interaction_type', '?')
            info_lines.append("")
            info_lines.append("Interaction")
            info_lines.append(f"  Type:      {interaction}")
            
            particle_energy = metadata.get('particle_energy', None)
            if particle_energy is not None:
                if particle_energy < 0:
                    info_lines.append("  Energy:    background (no MC truth)")
                else:
                    info_lines.append(f"  Energy:    {particle_energy:>8.4f} GeV")
            
            mom_x = metadata.get('main_track_momentum_x', None)
            mom_y = metadata.get('main_track_momentum_y', None)
            mom_z = metadata.get('main_track_momentum_z', None)
            mom_mag = metadata.get('main_track_momentum', None)
            if mom_mag is not None:
                info_lines.append("  Momentum:")
                info_lines.append(f"    |p|: {mom_mag:>8.4f} GeV/c")
                if mom_x is not None and mom_y is not None and mom_z is not None:
                    info_lines.append(f"    px:  {mom_x:>8.4f} GeV/c")
                    info_lines.append(f"    py:  {mom_y:>8.4f} GeV/c")
                    info_lines.append(f"    pz:  {mom_z:>8.4f} GeV/c")
        
        info_lines.append("")
        info_lines.append("Navigation")
        info_lines.append("  Left/Right : previous / next volume")
        info_lines.append("  Space      : next volume")
        info_lines.append("  q          : quit viewer")
        
        info_text = "\n".join(info_lines)
        
        self.ax_info.text(0.05, 0.98, info_text, transform=self.ax_info.transAxes,
                         fontsize=10, verticalalignment='top', fontfamily='monospace',
                         bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.2, pad=1))
        
        # Update main title
        title = f"Volume Image Viewer - {filename}"
        self.fig.suptitle(title, fontsize=13, fontweight='bold')
        
        # Update button states
        self.btn_prev.ax.set_visible(self.current_idx > 0)
        self.btn_next.ax.set_visible(self.current_idx < self.n_volumes - 1)
        
        self.fig.canvas.draw_idle()
        
        print(f"\n[{self.current_idx + 1}/{self.n_volumes}] {filename}")
        print(f"  Shape: {image.shape}, Range: [{image.min():.2f}, {image.max():.2f}]")
    
    def prev_volume(self, event=None):
        """Go to previous volume"""
        if self.current_idx > 0:
            self.current_idx -= 1
            self.display_current_volume()
    
    def next_volume(self, event=None):
        """Go to next volume"""
        if self.current_idx < self.n_volumes - 1:
            self.current_idx += 1
            self.display_current_volume()
    
    def on_key(self, event):
        """Handle keyboard shortcuts"""
        if event.key == 'left':
            self.prev_volume()
        elif event.key == 'right' or event.key == ' ':
            self.next_volume()
        elif event.key == 'q':
            plt.close(self.fig)


def find_volume_files(path_pattern):
    """Find all volume .npz files matching the pattern"""
    path = Path(path_pattern)
    
    if path.is_file() and path.suffix == '.npz':
        return [path]
    elif path.is_dir():
        # Find all .npz files in directory
        return list(path.glob('*volume*.npz'))
    else:
        # Try glob pattern
        files = glob.glob(str(path_pattern), recursive=False)
        npz_files = [Path(f) for f in files if f.endswith('.npz')]
        
        if not npz_files:
            # Try adding volume pattern
            parent = Path(path_pattern)
            if parent.is_dir():
                npz_files = list(parent.glob('*volume*.npz'))
        
        return npz_files


def main():
    parser = argparse.ArgumentParser(
        description='Interactive viewer for volume images with navigation',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  # View all volumes in a folder\n"
            "  %(prog)s data2/prod_es/volume_images_es_valid_test_bg_tick3_ch2_min2_tot3_e2p0/\n\n"
            "  # View specific pattern\n"
            "  %(prog)s \"data2/prod_es/volume_images_*/*volume00*.npz\"\n\n"
            "Navigation:\n"
            "  - Click 'Previous' / 'Next' buttons\n"
            "  - Use arrow keys: left / right\n"
            "  - Press Space for next\n"
            "  - Press 'q' to quit\n"
        )
    )
    
    parser.add_argument('path', 
                       help='Path to volume folder or file pattern')
    
    parser.add_argument('--no-tps', action='store_true',
                       help='Show clusters as colored dots instead of full TP images. '
                            'Main track=red, MARLEY=orange, Background=blue')
    
    args = parser.parse_args()
    
    # Check if uproot is available for cluster dots mode
    if args.no_tps and not HAS_UPROOT:
        print("Warning: --no-tps mode requires 'uproot' package")
        print("Install with: pip install uproot")
        print("Falling back to full TP display mode")
        args.no_tps = False
    
    # Find volume files
    volume_files = find_volume_files(args.path)
    
    if not volume_files:
        print(f"Error: No volume .npz files found matching: {args.path}")
        print("\nTry:")
        print(f"  python {sys.argv[0]} data2/prod_es/volume_images_*/")
        sys.exit(1)
    
    print(f"Found {len(volume_files)} volume files")
    
    # Launch viewer
    viewer = VolumeViewer(volume_files, show_tps=not args.no_tps)


if __name__ == '__main__':
    main()
