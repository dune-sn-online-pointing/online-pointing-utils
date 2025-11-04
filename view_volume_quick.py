#!/usr/bin/env python3
"""Interactive volume viewer for the multi-volume NPZ format
Navigate with arrow keys, space, or buttons
Shows both pentagon-rendered TPs and cluster scatter plot"""
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Button
from pathlib import Path
import sys

# Try to import uproot for cluster loading
try:
    import uproot
    HAS_UPROOT = True
except ImportError:
    HAS_UPROOT = False
    print("Warning: uproot not available. Install with: pip install uproot")

# Conversion factor from TDC counts to TPC ticks
TDC_TO_TPC_CONVERSION = 32

class VolumeViewer:
    def __init__(self, npz_file):
        self.npz_file = npz_file
        
        # Load data
        data = np.load(npz_file, allow_pickle=True)
        self.npz_path = Path(npz_file)
        self.npz_dir = self.npz_path.parent
        self.images = data['images']
        self.metadata = data['metadata']
        self.n_volumes = len(self.images)
        self.current_idx = 0

        # Determine ROOT file path for cluster loading
        self.root_file = None
        if self.n_volumes > 0:
            first_meta = self.metadata[0]
            source_root = first_meta.get('source_root_file') if isinstance(first_meta, dict) else None
            if source_root:
                resolved = Path(source_root)
                if not resolved.is_absolute():
                    resolved = (self.npz_dir / resolved).resolve()
                if resolved.exists():
                    self.root_file = str(resolved)

        if self.root_file is None:
            self.root_file = self._find_root_file(npz_file)

        print(f"\nFile: {self.npz_path.name}")
        print(f"Total volumes: {self.n_volumes}")
        if self.root_file and HAS_UPROOT:
            print(f"ROOT file: {Path(self.root_file).name}")
        print("Navigation: Left/Right arrows, Space (next), q (quit)")

        # Create figure with three panels (TP image, clusters, metadata)
        self.fig, self.axes = plt.subplots(1, 3, figsize=(20, 7))
        self.ax_tps, self.ax_clusters, self.ax_info = self.axes
        
        # Navigation buttons
        ax_prev = plt.axes([0.35, 0.02, 0.1, 0.04])
        ax_next = plt.axes([0.55, 0.02, 0.1, 0.04])
        self.btn_prev = Button(ax_prev, 'Previous')
        self.btn_next = Button(ax_next, 'Next')
        
        self.btn_prev.on_clicked(self.prev_volume)
        self.btn_next.on_clicked(self.next_volume)
        
        # Keyboard shortcuts
        self.fig.canvas.mpl_connect('key_press_event', self.on_key)
        
        # Display first volume
        self.colorbar = None
        self.display_current_volume()
        
        plt.tight_layout(rect=[0, 0.06, 1, 0.96])
        plt.show()
    
    def _find_root_file(self, npz_path):
        """Find the corresponding ROOT file with cluster information"""
        npz_name = Path(npz_path).stem
        
        # Remove _planeX suffix to get base name
        base_parts = npz_name.split('_')
        try:
            plane_idx = next(i for i, part in enumerate(base_parts) if part.startswith('plane'))
            base_name = '_'.join(base_parts[:plane_idx])
        except StopIteration:
            return None
        
        # Look for matched clusters ROOT file
        root_dir = volume_dir.parent
        
        # Try matched_clusters folder
        matched_dir = root_dir / f"matched_{base_name.split('_')[0]}_clusters_{base_name.split('_')[-1]}"
        if not matched_dir.exists():
            # Try variations
            for d in root_dir.glob("matched_clusters*"):
                if d.is_dir():
                    matched_dir = d
                    break
        
        if matched_dir.exists():
            # Look for ROOT file
            root_file = matched_dir / f"{base_name}.root"
            if root_file.exists():
                return str(root_file)
            
            # Try with _matched suffix
            root_file = matched_dir / f"{base_name}_matched.root"
            if root_file.exists():
                return str(root_file)
        
        return None
    
    def _load_clusters_for_volume(self, metadata):
        """Load clusters from ROOT file for the current volume"""
        if not HAS_UPROOT:
            return []

        try:
            root_path = metadata.get('source_root_file') if isinstance(metadata, dict) else None
            candidate_path = None

            if root_path:
                candidate = Path(root_path)
                if not candidate.is_absolute():
                    candidate = (self.npz_dir / candidate).resolve()
                if candidate.exists():
                    candidate_path = candidate

            if candidate_path is None and self.root_file:
                fallback_candidate = Path(self.root_file)
                if fallback_candidate.exists():
                    candidate_path = fallback_candidate

            if candidate_path is None or not candidate_path.exists():
                return []

            self.root_file = str(candidate_path)
            f = uproot.open(self.root_file)
            plane = metadata.get('plane', 'X')
            event = metadata.get('event')
            center_ch = metadata.get('center_channel', 0)
            center_time = metadata.get('center_time_tpc', 0)
            vol_size = metadata.get('volume_size_cm', 100.0)
            image_shape = metadata.get('image_shape', (208, 1242))
            
            # Volume bounds (assuming standard detector parameters)
            # WIRE_PITCH_COLLECTION_CM = 0.479, TIME_TICK_CM = 0.0805
            volume_half_ch = image_shape[0] / 2
            volume_half_time = image_shape[1] / 2
            ch_min = center_ch - volume_half_ch
            ch_max = center_ch + volume_half_ch
            time_min = center_time - volume_half_time
            time_max = center_time + volume_half_time
            
            clusters = []
            for directory in ['clusters', 'discarded']:
                tree_name = f'{directory}/clusters_tree_{plane}'
                if tree_name not in f:
                    continue
                
                tree = f[tree_name]
                
                # Load cluster data
                cluster_ids = tree['cluster_id'].array(library='np')
                events = tree['event'].array(library='np')
                tp_channels = tree['tp_detector_channel'].array(library='np')
                tp_times_tdc = tree['tp_time_start'].array(library='np')
                marley_fractions = tree['marley_tp_fraction'].array(library='np')
                main_flags = tree['is_main_cluster'].array(library='np') if 'is_main_cluster' in tree.keys() else None
                
                for i in range(len(cluster_ids)):
                    evt = events[i]
                    if hasattr(evt, '__len__') and len(evt) > 0:
                        evt = evt[0]
                    
                    # Filter by event
                    if event is not None and int(evt) != int(event):
                        continue
                    
                    ch_vals = tp_channels[i]
                    time_vals_tdc = tp_times_tdc[i]
                    
                    if len(ch_vals) == 0:
                        continue
                    
                    # Convert times from TDC to TPC ticks
                    time_vals = time_vals_tdc / TDC_TO_TPC_CONVERSION
                    
                    # Calculate cluster center
                    center_channel = np.mean(ch_vals)
                    center_time_tp = np.mean(time_vals)
                    
                    # Filter by volume bounds
                    if not (ch_min <= center_channel <= ch_max and
                            time_min <= center_time_tp <= time_max):
                        continue
                    
                    # Get cluster properties
                    cid = cluster_ids[i]
                    if hasattr(cid, '__len__') and len(cid) > 0:
                        cid = cid[0]
                    
                    marley_frac = marley_fractions[i]
                    if hasattr(marley_frac, '__len__') and len(marley_frac) > 0:
                        marley_frac = marley_frac[0]
                    
                    is_marley = float(marley_frac) > 0
                    is_main = False
                    if main_flags is not None:
                        main_flag = main_flags[i]
                        if hasattr(main_flag, '__len__') and len(main_flag) > 0:
                            main_flag = main_flag[0]
                        is_main = bool(main_flag)
                    
                    is_discarded = (directory == 'discarded')
                    
                    # Set display properties
                    if is_main:
                        color = '#FF0000'  # Red
                        size = 400
                        marker = '*'
                        label = 'Main Track'
                        zorder = 10
                    elif is_discarded:
                        color = '#666666'  # Gray
                        size = 90
                        marker = 'o'
                        label = 'Discarded'
                        zorder = 2
                    elif is_marley:
                        color = '#FF8800'  # Orange
                        size = 150
                        marker = 'o'
                        label = 'MARLEY'
                        zorder = 5
                    else:
                        color = '#0088FF'  # Blue
                        size = 80
                        marker = 'o'
                        label = 'Background'
                        zorder = 3
                    
                    clusters.append({
                        'cluster_id': int(cid),
                        'channel': center_channel,
                        'time': center_time_tp,
                        'n_tps': len(ch_vals),
                        'is_marley': is_marley,
                        'is_main': is_main,
                        'is_discarded': is_discarded,
                        'color': color,
                        'size': size,
                        'marker': marker,
                        'label': label,
                        'zorder': zorder
                    })
            
            return clusters
            
        except Exception as e:
            print(f"Error loading clusters: {e}")
            return []
    
    def display_current_volume(self):
        """Display the current volume"""
        # Get data
        image = self.images[self.current_idx]
        meta = self.metadata[self.current_idx]
        
        # Convert object dtype to float if needed
        if image.dtype == object:
            image = np.array(image, dtype=np.float32)
        
        # Load clusters if available
        clusters = self._load_clusters_for_volume(meta)
        
        # Clear axes
        self.ax_tps.clear()
        self.ax_clusters.clear()
        self.ax_info.clear()
        self.ax_info.axis('off')
        
        # Get volume bounds
        center_ch = meta.get('center_channel', 0)
        center_time = meta.get('center_time_tpc', 0)
        half_ch = image.shape[0] / 2
        half_time = image.shape[1] / 2
        extent = [center_time - half_time, center_time + half_time,
                  center_ch - half_ch, center_ch + half_ch]
        
        # LEFT PANEL: Pentagon-rendered TP image
        im = self.ax_tps.imshow(image, cmap='viridis', aspect='auto', 
                                interpolation='nearest', origin='lower',
                                extent=extent)
        
        # Update or create colorbar
        if self.colorbar is None:
            self.colorbar = plt.colorbar(im, ax=self.ax_tps, label='ADC (Pentagon Interpolation)', fraction=0.046)
        else:
            self.colorbar.update_normal(im)
        
        self.ax_tps.set_title(f'TPs (Pentagon Method)\nShape: {image.shape}',
                              fontsize=11, fontweight='bold')
        self.ax_tps.set_xlabel('Time (TPC ticks)', fontsize=10)
        self.ax_tps.set_ylabel('Channel', fontsize=10)
        self.ax_tps.grid(True, alpha=0.2, linestyle='--')
        
        # MIDDLE PANEL: Cluster scatter plot
        if clusters:
            # Separate clusters by type
            main_clusters = [c for c in clusters if c['is_main']]
            marley_clusters = [c for c in clusters if c['is_marley'] and not c['is_main'] and not c['is_discarded']]
            bg_clusters = [c for c in clusters if not c['is_marley'] and not c['is_discarded']]
            discarded_clusters = [c for c in clusters if c['is_discarded']]
            
            # Plot in order (background first, then MARLEY, then discarded, then main)
            if bg_clusters:
                times = [c['time'] for c in bg_clusters]
                channels = [c['channel'] for c in bg_clusters]
                self.ax_clusters.scatter(times, channels, s=80, c='#0088FF',
                                        alpha=0.7, edgecolors='black', linewidths=1,
                                        label=f'Background ({len(bg_clusters)})', zorder=3, marker='o')
            
            discarded_bg = [c for c in discarded_clusters if not c['is_marley']]
            discarded_marley = [c for c in discarded_clusters if c['is_marley']]

            if discarded_bg:
                times = [c['time'] for c in discarded_bg]
                channels = [c['channel'] for c in discarded_bg]
                self.ax_clusters.scatter(times, channels, s=90, c='#0088FF',
                                        alpha=0.5, edgecolors='black', linewidths=1.2,
                                        label=f'Background < E_cut ({len(discarded_bg)})', zorder=2, marker='o')

            if discarded_marley:
                times = [c['time'] for c in discarded_marley]
                channels = [c['channel'] for c in discarded_marley]
                self.ax_clusters.scatter(times, channels, s=90, c='#FF8800',
                                        alpha=0.5, edgecolors='black', linewidths=1.2,
                                        label=f'MARLEY < E_cut ({len(discarded_marley)})', zorder=2, marker='o')
            
            if marley_clusters:
                times = [c['time'] for c in marley_clusters]
                channels = [c['channel'] for c in marley_clusters]
                self.ax_clusters.scatter(times, channels, s=150, c='#FF8800',
                                        alpha=0.8, edgecolors='black', linewidths=1.5,
                                        label=f'MARLEY ({len(marley_clusters)})', zorder=5, marker='o')
            
            if main_clusters:
                times = [c['time'] for c in main_clusters]
                channels = [c['channel'] for c in main_clusters]
                self.ax_clusters.scatter(times, channels, s=400, c='#FF0000',
                                        alpha=0.9, edgecolors='black', linewidths=2,
                                        label=f'Main Track ({len(main_clusters)})', zorder=10, marker='*')
            
            self.ax_clusters.legend(loc='upper right', fontsize=9, framealpha=0.9)
            
            # Stats text
            n_kept = len(bg_clusters) + len(marley_clusters) + len(main_clusters)
            n_total = n_kept + len(discarded_clusters)
            n_marley = len(marley_clusters) + (1 if main_clusters and main_clusters[0]['is_marley'] else 0)
            marley_frac = n_marley / n_kept if n_kept > 0 else 0
            
            stats_text = f'Total: {n_total} clusters\nKept: {n_kept}\nDiscarded: {len(discarded_clusters)}\nMARLEY: {n_marley} ({marley_frac:.1%})'
            self.ax_clusters.text(0.02, 0.98, stats_text, transform=self.ax_clusters.transAxes,
                                 fontsize=9, verticalalignment='top',
                                 bbox=dict(boxstyle='round', facecolor='white', alpha=0.85))
        else:
            self.ax_clusters.text(0.5, 0.5, 'No cluster data\n(uproot not available or ROOT file not found)',
                                 transform=self.ax_clusters.transAxes, ha='center', va='center',
                                 fontsize=10, color='gray')
        
        self.ax_clusters.set_title('Cluster Centers', fontsize=11, fontweight='bold')
        self.ax_clusters.set_xlabel('Time (TPC ticks)', fontsize=10)
        self.ax_clusters.set_ylabel('Channel', fontsize=10)
        self.ax_clusters.grid(True, alpha=0.2, linestyle='--')
        self.ax_clusters.set_xlim(extent[0], extent[1])
        self.ax_clusters.set_ylim(extent[2], extent[3])
        
        # RIGHT PANEL: Metadata info
        info_lines = []
        info_lines.append(f"=== Volume {self.current_idx + 1}/{self.n_volumes} ===")
        info_lines.append("")
        info_lines.append(f"Event:   {meta.get('event', '?')}")
        info_lines.append(f"Plane:   {meta.get('plane', '?')}")
        info_lines.append(f"Type:    {meta.get('interaction_type', '?')}")
        info_lines.append("")
        info_lines.append("=== Volume Properties ===")
        info_lines.append(f"Center:  ({meta.get('center_channel', 0):.1f}, {meta.get('center_time_tpc', 0):.1f})")
        info_lines.append(f"Size:    {meta.get('volume_size_cm', 100):.0f} cm x {meta.get('volume_size_cm', 100):.0f} cm")
        info_lines.append(f"Shape:   {image.shape[0]} ch x {image.shape[1]} time")
        info_lines.append("")
        info_lines.append("=== Clusters (from metadata) ===")
        info_lines.append(f"Total:       {meta.get('n_clusters_in_volume', '?')}")
        info_lines.append(f"MARLEY:      {meta.get('n_marley_clusters', '?')}")
        info_lines.append(f"Background:  {meta.get('n_non_marley_clusters', '?')}")
        
        marley_frac = meta.get('n_marley_clusters', 0) / max(meta.get('n_clusters_in_volume', 1), 1)
        info_lines.append(f"MARLEY %:    {marley_frac:.1%}")
        info_lines.append("")
        
        info_lines.append("=== Main Track ===")
        particle_energy = meta.get('particle_energy', -1)
        if particle_energy >= 0:
            info_lines.append(f"Energy:  {particle_energy:.3f} MeV")
        else:
            info_lines.append("Energy:  background (no truth)")
        
        mom = meta.get('main_track_momentum', -1)
        if mom >= 0:
            info_lines.append(f"Momentum:")
            info_lines.append(f"  |p|: {mom*1000:.2f} MeV/c")
            info_lines.append(f"  px:  {meta.get('main_track_momentum_x', 0)*1000:.2f}")
            info_lines.append(f"  py:  {meta.get('main_track_momentum_y', 0)*1000:.2f}")
            info_lines.append(f"  pz:  {meta.get('main_track_momentum_z', 0)*1000:.2f}")
        
        info_lines.append("")
        info_lines.append("=== Image Statistics ===")
        info_lines.append(f"Min:      {image.min():8.2f}")
        info_lines.append(f"Max:      {image.max():8.2f}")
        info_lines.append(f"Mean:     {image.mean():8.2f}")
        info_lines.append(f"Std:      {image.std():8.2f}")
        info_lines.append(f"Non-zero: {(image > 0).sum():,} px")
        sparsity = (image == 0).sum() / image.size * 100
        info_lines.append(f"Sparsity: {sparsity:.1f}%")
        
        info_lines.append("")
        info_lines.append("=== Navigation ===")
        info_lines.append("Left/Right : prev/next")
        info_lines.append("Space      : next")
        info_lines.append("q          : quit")
        
        info_text = "\n".join(info_lines)
        self.ax_info.text(0.05, 0.98, info_text, transform=self.ax_info.transAxes,
                         fontsize=11, verticalalignment='top', fontfamily='monospace',
                         bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.3))
        
        # Update title
        filename = Path(self.npz_file).name
        if len(filename) > 80:
            filename = filename[:40] + "..." + filename[-37:]
        self.fig.suptitle(f'{filename} - Volume {self.current_idx + 1}/{self.n_volumes}', 
                         fontsize=13, fontweight='bold')
        
        # Update button states
        self.btn_prev.ax.set_visible(self.current_idx > 0)
        self.btn_next.ax.set_visible(self.current_idx < self.n_volumes - 1)
        
        self.fig.canvas.draw_idle()
        
        print(f"[{self.current_idx + 1}/{self.n_volumes}] MARLEY: {meta.get('n_marley_clusters', 0)}/{meta.get('n_clusters_in_volume', 0)} ({marley_frac:.1%}), Energy: {particle_energy:.2f} MeV")
    
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

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python view_volume_quick.py <npz_file>")
        print("\nExample:")
        print("  python view_volume_quick.py data2/prod_es/volume_images_*/prodmarley*.npz")
        sys.exit(1)
    
    npz_file = sys.argv[1]
    if not Path(npz_file).exists():
        print(f"Error: File not found: {npz_file}")
        sys.exit(1)
    
    viewer = VolumeViewer(npz_file)
