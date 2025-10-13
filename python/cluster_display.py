#!/usr/bin/env python3
"""
Interactive MARLEY Cluster Viewer - Python port of cluster_display.cpp

This application displays trigger primitive (TP) clusters from ROOT files with
multiple visualization modes (pentagon, triangle, rectangle) and interactive navigation.
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
from matplotlib.widgets import Button
from matplotlib.patches import Rectangle
import numpy as np
import uproot
from scipy.optimize import minimize_scalar


class Parameters:
    """Load and manage parameters from .dat files"""
    
    def __init__(self, params_dir: str = "parameters"):
        self.params_dir = Path(params_dir)
        self.params = {}
        self.load_parameters()
    
    def load_parameters(self):
        """Load parameters from all .dat files"""
        for dat_file in ['display.dat', 'timing.dat', 'geometry.dat']:
            file_path = self.params_dir / dat_file
            if file_path.exists():
                self._parse_dat_file(file_path)
    
    def _parse_dat_file(self, file_path: Path):
        """Parse a single .dat parameter file"""
        with open(file_path, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('<') and line.endswith('>'):
                    # Parse parameter line: < key = value >
                    content = line[1:-1].strip()
                    if '=' in content:
                        parts = content.split('=', 1)
                        key = parts[0].strip()
                        value_str = parts[1].strip()
                        # Try to convert to appropriate type
                        try:
                            if '.' in value_str or 'e' in value_str.lower():
                                value = float(value_str)
                            else:
                                value = int(value_str)
                        except ValueError:
                            value = value_str
                        self.params[key] = value
    
    def get(self, key: str, default=None):
        """Get parameter value by key"""
        return self.params.get(key, default)


class ClusterItem:
    """Represents a single cluster or event to display"""
    
    def __init__(self):
        self.plane = ""
        self.ch = []
        self.tstart = []
        self.sot = []
        self.stopeak = []
        self.adc_peak = []
        self.adc_integral = []
        self.label = ""
        self.interaction = ""
        self.enu = 0.0
        self.total_charge = 0.0
        self.total_energy = 0.0
        self.is_event = False
        self.event_id = -1
        self.n_clusters = 0
        self.marley_only = False


class DrawingAlgorithms:
    """Pentagon, triangle, and rectangle drawing algorithms"""
    
    @staticmethod
    def polygon_area(vertices: List[Tuple[float, float]]) -> float:
        """Calculate polygon area using Shoelace formula"""
        n = len(vertices)
        if n < 3:
            return 0.0
        sum1, sum2 = 0.0, 0.0
        for i in range(n):
            j = (i + 1) % n
            sum1 += vertices[i][0] * vertices[j][1]
            sum2 += vertices[j][0] * vertices[i][1]
        return 0.5 * abs(sum1 - sum2)
    
    @staticmethod
    def calculate_pentagon_params(time_start: float, time_peak: float, time_end: float,
                                  adc_peak: float, adc_integral: float,
                                  threshold: float) -> Optional[Dict]:
        """
        Calculate pentagon parameters using plateau rectangle baseline and optimization.
        
        The pentagon sits on top of a rectangular plateau at the threshold level.
        The offset area (rectangular baseline) is excluded from optimization.
        
        Args:
            time_start: Start time of the trigger primitive
            time_peak: Peak time (absolute)
            time_end: End time
            adc_peak: Peak ADC value (measured from baseline = 0)
            adc_integral: Total ADC integral (target area)
            threshold: Threshold value for this plane
        
        Returns:
            Dictionary with pentagon parameters or None if optimization fails
        """
        # Calculate fixed time positions at 0.5 the distance
        time_over_threshold = time_end - time_start
        t1 = round(time_start + 0.5 * (time_peak - time_start))
        t2 = round(time_peak + 0.5 * (time_end - time_peak))
        
        # Calculate rectangular plateau offset area (from baseline to threshold)
        offset_area = threshold * time_over_threshold
        
        # Define objective function to minimize
        def objective(h):
            """Calculate absolute difference between pentagon total area and target area."""
            vertices = [
                (time_start, threshold),
                (t1, threshold + h),
                (time_peak, adc_peak),  # Peak is already measured from baseline
                (t2, threshold + h),
                (time_end, threshold)
            ]
            pentagon_area_above_threshold = DrawingAlgorithms.polygon_area(vertices)
            pentagon_total = pentagon_area_above_threshold + offset_area
            return abs(pentagon_total - adc_integral)
        
        # Optimize h to minimize the difference
        # h can range from 0 to (adc_peak - threshold) since it's added to threshold
        max_h = adc_peak - threshold
        if max_h <= 0:
            return None
        
        result = minimize_scalar(objective, bounds=(0, max_h), method='bounded')
        h_opt = result.x
        
        # Pentagon vertices with optimized h (all coordinates measured from baseline = 0)
        y_intermediate = threshold + h_opt
        
        return {
            'time_int_rise': t1,
            'time_int_fall': t2,
            'h_int_rise': y_intermediate,
            'h_int_fall': y_intermediate,
            'valid': True,
            'h_opt': h_opt,
            'threshold': threshold,
            'offset_area': offset_area
        }
    
    @staticmethod
    def fill_histogram_pentagon(hist: np.ndarray, ch_idx: int, time_start: int,
                               time_peak: int, samples_over_threshold: int,
                               adc_peak: float, adc_integral: float,
                               threshold_adc: float,
                               tmin: int, ch_map: Dict[int, int]):
        """Fill histogram with pentagon shape (extended range)"""
        time_end = time_start + samples_over_threshold
        
        # Clamp peak time
        time_peak = max(time_start, min(time_end, time_peak))
        
        params = DrawingAlgorithms.calculate_pentagon_params(
            time_start, time_peak, time_end, adc_peak, adc_integral, threshold_adc
        )
        
        if not params:
            # Fallback to triangle
            DrawingAlgorithms.fill_histogram_triangle(
                hist, ch_idx, time_start, samples_over_threshold,
                time_peak - time_start, adc_peak, threshold_adc, tmin, ch_map
            )
            return
        
        t_int_rise = int(round(params['time_int_rise']))
        t_int_fall = int(round(params['time_int_fall']))
        threshold = params['threshold']
        
        # Extended range (1 tick before/after)
        extended_start = time_start - 1
        extended_end = time_end + 1
        
        for t in range(extended_start, extended_end):
            intensity = 0.0
            
            if t < time_start:
                # Extended base before time_start
                span = t_int_rise - time_start
                if span > 0:
                    frac = (t - time_start) / span
                    intensity = threshold + frac * (params['h_int_rise'] - threshold)
                    if intensity < threshold_adc * 0.5:
                        intensity = 0.0
            elif t < t_int_rise:
                # Segment 1: rising from threshold to h_int_rise
                span = t_int_rise - time_start
                if span > 0:
                    frac = (t - time_start) / span
                    intensity = threshold + frac * (params['h_int_rise'] - threshold)
            elif t < time_peak:
                # Segment 2: rising from h_int_rise to adc_peak
                span = time_peak - t_int_rise
                if span > 0:
                    frac = (t - t_int_rise) / span
                    intensity = params['h_int_rise'] + frac * (adc_peak - params['h_int_rise'])
                else:
                    intensity = adc_peak
            elif t == time_peak:
                intensity = adc_peak
            elif t <= t_int_fall:
                # Segment 3: falling from adc_peak to h_int_fall
                span = t_int_fall - time_peak
                if span > 0:
                    frac = (t - time_peak) / span
                    intensity = adc_peak - frac * (adc_peak - params['h_int_fall'])
                else:
                    intensity = params['h_int_fall']
            elif t < time_end:
                # Segment 4: falling from h_int_fall to threshold
                span = time_end - t_int_fall
                if span > 0:
                    frac = (t - t_int_fall) / span
                    intensity = params['h_int_fall'] - frac * (params['h_int_fall'] - threshold)
            else:
                # Extended base after time_end
                span = time_end - t_int_fall
                if span > 0:
                    frac = (t - t_int_fall) / span
                    intensity = params['h_int_fall'] - frac * (params['h_int_fall'] - threshold)
                    if intensity < threshold_adc * 0.5:
                        intensity = 0.0
            
            if intensity > 0:
                t_idx = t - tmin
                if 0 <= t_idx < hist.shape[0] and 0 <= ch_idx < hist.shape[1]:
                    hist[t_idx, ch_idx] = max(hist[t_idx, ch_idx], intensity)
    
    @staticmethod
    def fill_histogram_triangle(hist: np.ndarray, ch_idx: int, time_start: int,
                               samples_over_threshold: int, samples_to_peak: int,
                               adc_peak: float, threshold_adc: float,
                               tmin: int, ch_map: Dict[int, int]):
        """Fill histogram with triangle shape"""
        time_end = time_start + max(1, samples_over_threshold)
        peak_time = time_start + samples_to_peak
        
        for t in range(time_start, time_end):
            if t <= peak_time:
                # Rising edge
                if peak_time != time_start:
                    frac = (t - time_start) / (peak_time - time_start)
                    intensity = threshold_adc + frac * (adc_peak - threshold_adc)
                else:
                    intensity = adc_peak
            else:
                # Falling edge
                fall_span = (time_end - 1) - peak_time
                if fall_span > 0:
                    frac = (t - peak_time) / fall_span
                    intensity = adc_peak - frac * (adc_peak - threshold_adc)
                else:
                    intensity = adc_peak
            
            t_idx = t - tmin
            if 0 <= t_idx < hist.shape[0] and 0 <= ch_idx < hist.shape[1]:
                hist[t_idx, ch_idx] = max(hist[t_idx, ch_idx], intensity)
    
    @staticmethod
    def fill_histogram_rectangle(hist: np.ndarray, ch_idx: int, time_start: int,
                                samples_over_threshold: int, adc_integral: float,
                                tmin: int, ch_map: Dict[int, int]):
        """Fill histogram with rectangle (uniform intensity)"""
        if samples_over_threshold > 0:
            uniform_intensity = adc_integral / samples_over_threshold
        else:
            uniform_intensity = 0.0
        
        time_end = time_start + samples_over_threshold
        
        for t in range(time_start, time_end):
            t_idx = t - tmin
            if 0 <= t_idx < hist.shape[0] and 0 <= ch_idx < hist.shape[1]:
                hist[t_idx, ch_idx] = max(hist[t_idx, ch_idx], uniform_intensity)


class ClusterViewer:
    """Interactive cluster viewer application"""
    
    def __init__(self, clusters_file: str, mode: str = "clusters",
                 draw_mode: str = "pentagon", only_marley: bool = False,
                 params_dir: str = "parameters"):
        self.clusters_file = clusters_file
        self.mode = mode.lower()
        self.draw_mode = draw_mode.lower()
        self.only_marley = only_marley
        self.params = Parameters(params_dir)
        
        self.items: List[ClusterItem] = []
        self.current_idx = 0
        
        self.fig = None
        self.ax = None
        self.im = None
        self.btn_prev = None
        self.btn_next = None
        
        self.load_clusters()
    
    def load_clusters(self):
        """Load clusters from ROOT file"""
        print(f"Loading clusters from: {self.clusters_file}")
        
        with uproot.open(self.clusters_file) as f:
            # Try to find clusters directory
            try:
                clusters_dir = f['clusters']
            except KeyError:
                clusters_dir = f
            
            # Read from U, V, X planes
            for plane in ['U', 'V', 'X']:
                tree_name = f'clusters_tree_{plane}'
                try:
                    tree = clusters_dir[tree_name]
                except KeyError:
                    continue
                
                self._read_plane_clusters(tree, plane)
        
        print(f"Loaded {len(self.items)} MARLEY clusters total (all planes)")
    
    def _read_plane_clusters(self, tree, plane: str):
        """Read clusters from a single plane tree"""
        # Read all branches
        branches = [
            'n_tps', 'true_neutrino_energy', 'total_charge', 'total_energy',
            'true_label', 'true_interaction', 'event',
            'tp_detector_channel', 'tp_time_start', 'tp_samples_over_threshold',
            'tp_samples_to_peak', 'tp_adc_peak', 'tp_adc_integral'
        ]
        
        arrays = tree.arrays(branches, library='np')
        n_entries = len(arrays['event'])
        
        if self.mode == 'clusters':
            # Individual cluster mode
            for i in range(n_entries):
                label = arrays['true_label'][i] if 'true_label' in arrays else ''
                if isinstance(label, bytes):
                    label = label.decode('utf-8')
                
                # Filter for MARLEY clusters
                if 'marley' not in label.lower():
                    continue
                
                item = ClusterItem()
                item.plane = plane
                item.label = label
                
                # Handle interaction string (may be bytes or str)
                interaction = arrays['true_interaction'][i] if 'true_interaction' in arrays else ''
                if isinstance(interaction, bytes):
                    interaction = interaction.decode('utf-8')
                item.interaction = interaction
                
                item.enu = arrays['true_neutrino_energy'][i] if 'true_neutrino_energy' in arrays else 0.0
                item.total_charge = arrays['total_charge'][i] if 'total_charge' in arrays else 0.0
                item.total_energy = arrays['total_energy'][i] if 'total_energy' in arrays else 0.0
                item.event_id = arrays['event'][i] if 'event' in arrays else -1
                
                # TP data
                item.ch = list(arrays['tp_detector_channel'][i]) if 'tp_detector_channel' in arrays else []
                item.tstart = [int(ts / 32) for ts in arrays['tp_time_start'][i]] if 'tp_time_start' in arrays else []  # Convert to TPC ticks
                item.sot = list(arrays['tp_samples_over_threshold'][i]) if 'tp_samples_over_threshold' in arrays else []
                item.stopeak = list(arrays['tp_samples_to_peak'][i]) if 'tp_samples_to_peak' in arrays else []
                item.adc_peak = list(arrays['tp_adc_peak'][i]) if 'tp_adc_peak' in arrays else []
                item.adc_integral = list(arrays['tp_adc_integral'][i]) if 'tp_adc_integral' in arrays else []
                
                self.items.append(item)
        else:
            # Event aggregation mode
            events = {}
            for i in range(n_entries):
                label = arrays['true_label'][i] if 'true_label' in arrays else ''
                if isinstance(label, bytes):
                    label = label.decode('utf-8')
                
                if self.only_marley and 'marley' not in label.lower():
                    continue
                
                evt = arrays['event'][i]
                if evt not in events:
                    item = ClusterItem()
                    item.is_event = True
                    item.event_id = evt
                    item.plane = plane
                    item.marley_only = self.only_marley
                    item.n_clusters = 0
                    events[evt] = item
                
                events[evt].n_clusters += 1
                events[evt].ch.extend(list(arrays['tp_detector_channel'][i]))
                events[evt].tstart.extend([int(ts / 32) for ts in arrays['tp_time_start'][i]])
                events[evt].sot.extend(list(arrays['tp_samples_over_threshold'][i]))
                events[evt].stopeak.extend(list(arrays['tp_samples_to_peak'][i]))
                events[evt].adc_peak.extend(list(arrays['tp_adc_peak'][i]))
                events[evt].adc_integral.extend(list(arrays['tp_adc_integral'][i]))
            
            # Keep only events with > 1 TP
            for evt, item in events.items():
                if len(item.ch) > 1:
                    self.items.append(item)
    
    def draw_current(self):
        """Draw the current cluster/event"""
        if not self.items or self.current_idx >= len(self.items):
            return
        
        item = self.items[self.current_idx]
        n_tps = min(len(item.ch), len(item.tstart), len(item.sot))
        
        if n_tps == 0:
            return
        
        # Determine axis ranges and create channel mapping
        tmin, tmax = float('inf'), float('-inf')
        unique_channels = sorted(set(item.ch[:n_tps]))
        
        for i in range(n_tps):
            ts = item.tstart[i]
            te = item.tstart[i] + item.sot[i]
            tmin = min(tmin, ts)
            tmax = max(tmax, te)
        
        ch_to_idx = {ch: idx for idx, ch in enumerate(unique_channels)}
        
        # Create histogram with padding
        pad_bins = 2
        n_ch = len(unique_channels)
        n_t = int(tmax - tmin) + 1
        
        hist = np.zeros((n_t + 2 * pad_bins, n_ch + 2 * pad_bins))
        
        # Get threshold based on plane
        threshold_adc = 60.0
        if item.plane == 'U':
            threshold_adc = self.params.get('display.threshold_adc_u', 70.0)
        elif item.plane == 'V':
            threshold_adc = self.params.get('display.threshold_adc_v', 70.0)
        elif item.plane == 'X':
            threshold_adc = self.params.get('display.threshold_adc_x', 60.0)
        
        # Fill histogram based on draw mode
        for i in range(n_tps):
            ts = item.tstart[i]
            tot = item.sot[i]
            ch_actual = item.ch[i]
            ch_idx = ch_to_idx[ch_actual] + pad_bins
            
            samples_to_peak = item.stopeak[i] if i < len(item.stopeak) else (tot // 2 if tot > 0 else 0)
            peak_adc = item.adc_peak[i] if i < len(item.adc_peak) else 200
            peak_time = ts + samples_to_peak
            adc_integral = item.adc_integral[i] if i < len(item.adc_integral) else peak_adc * tot // 2
            
            if self.draw_mode == 'pentagon':
                DrawingAlgorithms.fill_histogram_pentagon(
                    hist, ch_idx, ts, peak_time, tot, peak_adc, adc_integral,
                    threshold_adc, int(tmin), ch_to_idx
                )
            elif self.draw_mode == 'triangle':
                DrawingAlgorithms.fill_histogram_triangle(
                    hist, ch_idx, ts, tot, samples_to_peak, peak_adc,
                    threshold_adc, int(tmin), ch_to_idx
                )
            else:  # rectangle
                DrawingAlgorithms.fill_histogram_rectangle(
                    hist, ch_idx, ts, tot, adc_integral, int(tmin), ch_to_idx
                )
        
        # Clear and redraw
        self.ax.clear()
        
        # Apply threshold mask
        hist_masked = np.ma.masked_where(hist < threshold_adc, hist)
        
        # Plot with viridis colormap
        extent = [
            -pad_bins - 0.5, n_ch + pad_bins - 0.5,
            int(tmin) - pad_bins - 0.5, int(tmax) + pad_bins + 0.5
        ]
        
        self.im = self.ax.imshow(
            hist_masked, aspect='auto', origin='lower',
            extent=extent, cmap='viridis', interpolation='nearest'
        )
        
        # Set labels
        self.ax.set_xlabel('channel (contiguous)')
        self.ax.set_ylabel('time [ticks]')
        
        # Create X-axis labels with actual channel numbers
        xticks = [i + pad_bins for i in range(len(unique_channels))]
        xticklabels = [str(ch) for ch in unique_channels]
        self.ax.set_xticks(xticks)
        self.ax.set_xticklabels(xticklabels, rotation=90, fontsize=8)
        
        # Add secondary axes for cm
        wire_pitch_cm = self.params.get('geometry.wire_pitch_collection_cm', 0.479)
        if item.plane in ['U', 'V']:
            wire_pitch_cm = self.params.get('geometry.wire_pitch_induction_diagonal_cm', 0.5)
        time_tick_cm = self.params.get('timing.time_tick_cm', 0.0805)
        
        # Add secondary Y axis (right side, inside plot)
        ax_y_cm = self.ax.twinx()
        ax_y_cm.set_ylim(self.ax.get_ylim()[0] * time_tick_cm, self.ax.get_ylim()[1] * time_tick_cm)
        ax_y_cm.set_ylabel('drift [cm]')
        
        # Title
        if item.is_event:
            title = f"Event {item.event_id} | plane {item.plane} | nTPs={n_tps} | nClusters={item.n_clusters}\n"
            title += f"mode=events, filter={'MARLEY-only' if item.marley_only else 'All clusters'}"
        else:
            title = f"Cluster {self.current_idx + 1}/{len(self.items)} | plane {item.plane}"
            if item.event_id >= 0:
                title += f" | event {item.event_id}"
            title += f" | nTPs={n_tps}\n"
            title += f"E_nu={item.enu:.1f} MeV, total_charge={item.total_charge:.0f}, total_energy={item.total_energy:.0f}"
        
        self.ax.set_title(title, fontsize=10)
        
        # Add colorbar
        if hasattr(self, 'cbar') and self.cbar is not None:
            try:
                self.cbar.remove()
            except:
                pass
        self.cbar = plt.colorbar(self.im, ax=self.ax, label=f'ADC ({self.draw_mode} model)')
        
        self.ax.grid(True, alpha=0.3)
        self.fig.canvas.draw_idle()
    
    def prev_cluster(self, event):
        """Navigate to previous cluster"""
        if self.current_idx > 0:
            self.current_idx -= 1
            self.draw_current()
    
    def next_cluster(self, event):
        """Navigate to next cluster"""
        if self.current_idx < len(self.items) - 1:
            self.current_idx += 1
            self.draw_current()
    
    def run(self):
        """Start the interactive viewer"""
        if not self.items:
            print("No clusters to display!")
            return
        
        # Create figure
        self.fig = plt.figure(figsize=(14, 8))
        
        # Create axes - main plot takes most of the space
        self.ax = self.fig.add_axes([0.1, 0.15, 0.8, 0.75])
        
        # Create navigation buttons
        ax_prev = self.fig.add_axes([0.3, 0.02, 0.15, 0.05])
        ax_next = self.fig.add_axes([0.55, 0.02, 0.15, 0.05])
        
        self.btn_prev = Button(ax_prev, 'Prev')
        self.btn_next = Button(ax_next, 'Next')
        
        self.btn_prev.on_clicked(self.prev_cluster)
        self.btn_next.on_clicked(self.next_cluster)
        
        # Draw initial cluster
        self.draw_current()
        
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description='Interactive MARLEY Cluster viewer (Python version)'
    )
    parser.add_argument('--clusters-file', required=True,
                       help='Input clusters ROOT file')
    parser.add_argument('--mode', default='clusters', choices=['clusters', 'events'],
                       help='Display mode (default: clusters)')
    parser.add_argument('--draw-mode', default='pentagon',
                       choices=['triangle', 'pentagon', 'rectangle'],
                       help='Drawing mode (default: pentagon)')
    parser.add_argument('--only-marley', action='store_true',
                       help='In events mode, show only MARLEY clusters')
    parser.add_argument('-j', '--json', help='JSON with input and parameters')
    parser.add_argument('--params-dir', default='parameters',
                       help='Directory containing parameter .dat files')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Verbose output')
    
    args = parser.parse_args()
    
    # Override with JSON if provided
    if args.json:
        with open(args.json, 'r') as f:
            config = json.load(f)
            if 'clusters_file' in config:
                args.clusters_file = config['clusters_file']
            if 'mode' in config:
                args.mode = config['mode']
            if 'draw_mode' in config:
                args.draw_mode = config['draw_mode']
            if 'only_marley' in config:
                args.only_marley = config['only_marley']
    
    print(f"Input clusters file: {args.clusters_file}")
    print(f"Display mode: {args.mode}")
    print(f"Draw mode: {args.draw_mode}")
    print(f"Only MARLEY in events mode: {args.only_marley}")
    
    viewer = ClusterViewer(
        args.clusters_file,
        mode=args.mode,
        draw_mode=args.draw_mode,
        only_marley=args.only_marley,
        params_dir=args.params_dir
    )
    
    viewer.run()


if __name__ == '__main__':
    main()
