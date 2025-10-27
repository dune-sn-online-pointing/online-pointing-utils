#!/usr/bin/env python3
"""
Plot the results of the energy cut scan study.
Shows background removal, MARLEY signal retention, and main track retention.
"""

import numpy as np
import matplotlib.pyplot as plt
import sys

def load_data(filename='energy_cut_scan_data.txt'):
    """Load the extracted cluster statistics."""
    
    cc_data = []
    es_data = []
    current_section = None
    
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith('#') or not line:
                continue
            
            if line == 'CC_DATA:':
                current_section = 'cc'
                continue
            elif line == 'ES_DATA:':
                current_section = 'es'
                continue
            
            # Parse data line: energy_cut marley background main_track total found
            parts = line.split()
            if len(parts) == 6:
                data_point = {
                    'energy_cut': float(parts[0]),
                    'marley': int(parts[1]),
                    'background': int(parts[2]),
                    'main_track': int(parts[3]),
                    'total': int(parts[4]),
                    'found': bool(int(parts[5]))
                }
                
                if current_section == 'cc':
                    cc_data.append(data_point)
                elif current_section == 'es':
                    es_data.append(data_point)
    
    return cc_data, es_data


def calculate_metrics(data):
    """Calculate efficiency metrics relative to baseline (energy_cut=0)."""
    
    if not data or not data[0]['found']:
        print("Warning: No baseline data found")
        return None
    
    # Get baseline (energy_cut = 0)
    baseline = data[0]
    baseline_marley = baseline['marley']
    baseline_background = baseline['background']
    baseline_main_track = baseline['main_track']
    
    if baseline_marley == 0 or baseline_background == 0:
        print("Warning: Zero baseline counts")
        return None
    
    metrics = {
        'energy_cuts': [],
        'background_removed': [],
        'marley_retained': [],
        'main_track_retention': []
    }
    
    for d in data:
        if not d['found']:
            continue
        
        metrics['energy_cuts'].append(d['energy_cut'])
        
        # Fraction of background removed (higher is better)
        bg_removed = 1.0 - (d['background'] / baseline_background)
        metrics['background_removed'].append(bg_removed * 100)  # Convert to percentage
        
        # Fraction of MARLEY retained (higher is better)
        marley_retained = (d['marley'] / baseline_marley) if baseline_marley > 0 else 0.0
        metrics['marley_retained'].append(marley_retained * 100)
        
        # Main track retention (higher is better)
        if baseline_main_track > 0:
            main_retention = d['main_track'] / baseline_main_track
            metrics['main_track_retention'].append(main_retention * 100)
        else:
            metrics['main_track_retention'].append(0)
    
    return metrics


def plot_results(cc_metrics, es_metrics, output_prefix='energy_cut_analysis'):
    """Create plots for CC and ES separately, each with all three variables."""
    
    # Plot CC results - all three metrics on one plot
    if cc_metrics:
        fig, ax = plt.subplots(1, 1, figsize=(12, 8))
        fig.suptitle('Energy Cut Study - CC Sample (View X)', fontsize=16, fontweight='bold', y=0.98)
        
        # Plot all three metrics
        ax.plot(cc_metrics['energy_cuts'], cc_metrics['background_removed'], 
                'o-', color='green', linewidth=2.5, markersize=10, label='Background Removed')
        ax.plot(cc_metrics['energy_cuts'], cc_metrics['marley_retained'], 
                's-', color='red', linewidth=2.5, markersize=10, label='MARLEY Retained')
        ax.plot(cc_metrics['energy_cuts'], cc_metrics['main_track_retention'], 
                '^-', color='blue', linewidth=2.5, markersize=10, label='Main Track Retention')
        
        ax.set_xlabel('Energy Cut (MeV)', fontsize=14, fontweight='bold')
        ax.set_ylabel('Percentage (%)', fontsize=14, fontweight='bold')
        ax.set_yticks(np.arange(0, 106, 10))
        # Annotate numeric values above each marker for the three metrics
        for x, y in zip(cc_metrics['energy_cuts'], cc_metrics['background_removed']):
            ax.annotate(f"{y:.1f}%", xy=(x, y), xytext=(0, 8), textcoords='offset points',
                ha='center', va='bottom', fontsize=12, color='green')
        for x, y in zip(cc_metrics['energy_cuts'], cc_metrics['marley_retained']):
            ax.annotate(f"{y:.1f}%", xy=(x, y), xytext=(0, -18), textcoords='offset points',
                ha='center', va='bottom', fontsize=12, color='red')
        for x, y in zip(cc_metrics['energy_cuts'], cc_metrics['main_track_retention']):
            ax.annotate(f"{y:.1f}%", xy=(x, y), xytext=(0, 8), textcoords='offset points',
                ha='center', va='bottom', fontsize=12, color='blue')
        ax.legend(fontsize=16, loc='best', framealpha=0.9)
        ax.grid(True, alpha=0.9, linestyle='--')
        ax.set_ylim([0, 105])
        ax.tick_params(labelsize=12)
        
        plt.tight_layout()
        cc_filename = f'{output_prefix}_cc.png'
        plt.savefig(cc_filename, dpi=150, bbox_inches='tight')
        print(f"CC plot saved to {cc_filename}")
        plt.close()
    
    # Plot ES results - all three metrics on one plot
    if es_metrics:
        fig, ax = plt.subplots(1, 1, figsize=(12, 8))
        fig.suptitle('Energy Cut Study - ES Sample (View X)', fontsize=16, fontweight='bold', y=0.98)
        
        # Plot all three metrics
        ax.plot(es_metrics['energy_cuts'], es_metrics['background_removed'], 
                'o-', color='green', linewidth=2.5, markersize=10, label='Background Removed')
        ax.plot(es_metrics['energy_cuts'], es_metrics['marley_retained'], 
                's-', color='red', linewidth=2.5, markersize=10, label='MARLEY Retained')
        ax.plot(es_metrics['energy_cuts'], es_metrics['main_track_retention'], 
                '^-', color='blue', linewidth=2.5, markersize=10, label='Main Track Retention')
        
        ax.set_xlabel('Energy Cut (MeV)', fontsize=14, fontweight='bold')
        ax.set_ylabel('Percentage (%)', fontsize=14, fontweight='bold')
        ax.set_yticks(np.arange(0, 106, 10))
                # Annotate numeric values above each marker for the three metrics
        for x, y in zip(es_metrics['energy_cuts'], es_metrics['background_removed']):
            ax.annotate(f"{y:.1f}%", xy=(x, y), xytext=(0, 8), textcoords='offset points',
                ha='center', va='bottom', fontsize=12, color='green')
        for x, y in zip(es_metrics['energy_cuts'], es_metrics['marley_retained']):
            ax.annotate(f"{y:.1f}%", xy=(x, y), xytext=(0, -18), textcoords='offset points',
                ha='center', va='bottom', fontsize=12, color='red')
        for x, y in zip(es_metrics['energy_cuts'], es_metrics['main_track_retention']):
            ax.annotate(f"{y:.1f}%", xy=(x, y), xytext=(0, 8), textcoords='offset points',
                ha='center', va='bottom', fontsize=12, color='blue')
        ax.legend(fontsize=16, loc='best', framealpha=0.9)
        ax.grid(True, alpha=0.9, linestyle='--')
        ax.set_ylim([0, 105])
        ax.tick_params(labelsize=12)
        
        plt.tight_layout()
        es_filename = f'{output_prefix}_es.png'
        plt.savefig(es_filename, dpi=150, bbox_inches='tight')
        print(f"ES plot saved to {es_filename}")
        plt.close()


def calculate_signal_to_background(data):
    """Calculate signal to background ratio for each energy cut."""
    
    ratios = {
        'energy_cuts': [],
        'signal_to_background': []
    }
    
    for d in data:
        if not d['found']:
            continue
        
        ratios['energy_cuts'].append(d['energy_cut'])
        
        # Calculate S/B ratio (handle case where background is 0)
        if d['background'] > 0:
            s_b_ratio = d['marley'] / d['background']
        else:
            # If no background, set a very high value (or use marley count)
            s_b_ratio = d['marley'] if d['marley'] > 0 else 0
        
        ratios['signal_to_background'].append(s_b_ratio)
    
    return ratios


def plot_signal_to_background(cc_data, es_data, output_prefix='energy_cut_analysis'):
    """Create a plot showing signal-to-background ratio vs energy cut."""
    
    cc_ratios = calculate_signal_to_background(cc_data)
    es_ratios = calculate_signal_to_background(es_data)
    
    fig, ax = plt.subplots(1, 1, figsize=(12, 8))
    fig.suptitle('Signal-to-Background Ratio vs Energy Cut (View X)', 
                 fontsize=16, fontweight='bold', y=0.98)
    
    # Plot both samples
    ax.plot(cc_ratios['energy_cuts'], cc_ratios['signal_to_background'], 
            'o-', color='darkred', linewidth=2.5, markersize=10, label='CC Sample')
    ax.plot(es_ratios['energy_cuts'], es_ratios['signal_to_background'], 
            's-', color='darkblue', linewidth=2.5, markersize=10, label='ES Sample')
    ax.set_xlim(0.8, 3.1)
    # Add a marker at y=1 (centered at x=2 MeV) to highlight S/B = 1
    ax.scatter([2.0], [1.0], marker='X', color='gray', s=80, zorder=5, label='_nolegend_')
    
    ax.set_xlabel('Energy Cut (MeV)', fontsize=14, fontweight='bold')
    ax.set_ylabel('Signal / Background Ratio', fontsize=14, fontweight='bold')

    # Annotate the numeric value above each plotted point
    for x, y in zip(cc_ratios['energy_cuts'], cc_ratios['signal_to_background']):
        if y is None:
            continue
        ax.annotate(f"{y:.2f}", xy=(x, y), xytext=(-8, 34), textcoords='offset points',
                    ha='center', va='bottom', fontsize=14, color='darkred')

    for x, y in zip(es_ratios['energy_cuts'], es_ratios['signal_to_background']):
        if y is None:
            continue
        ax.annotate(f"{y:.2f}", xy=(x, y), xytext=(-8, 55), textcoords='offset points',
                    ha='center', va='bottom', fontsize=14, color='darkblue')
    ax.legend(fontsize=24, loc='best', framealpha=0.9)
    ax.grid(True, alpha=0.3, linestyle='--')
    ax.set_ylim(bottom=0)
    ax.tick_params(labelsize=12)
    
    # Add a horizontal line at S/B = 1
    ax.axhline(y=1.0, color='gray', linestyle=':', linewidth=2, alpha=0.7, label='S/B = 1')
    ax.legend(fontsize=20, loc='best', framealpha=0.9)
    ax.set_ylim(0, 20)
    
    plt.tight_layout()
    sb_filename = f'{output_prefix}_signal_to_background.png'
    plt.savefig(sb_filename, dpi=150, bbox_inches='tight')
    print(f"Signal-to-background plot saved to {sb_filename}")
    plt.close()
    
    return cc_ratios, es_ratios


def print_summary(cc_metrics, es_metrics):
    """Print a summary of the results."""
    
    print("\n" + "="*60)
    print("ENERGY CUT STUDY SUMMARY")
    print("="*60)
    
    if cc_metrics:
        print("\nCC Sample (View X):")
        print(f"  Energy cuts tested: {cc_metrics['energy_cuts']}")
        print(f"  Background removed at 5 MeV: {cc_metrics['background_removed'][-1]:.1f}%")
        print(f"  MARLEY retained at 5 MeV: {cc_metrics['marley_retained'][-1]:.1f}%")
        print(f"  Main track retention at 5 MeV: {cc_metrics['main_track_retention'][-1]:.1f}%")
    
    if es_metrics:
        print("\nES Sample (View X):")
        print(f"  Energy cuts tested: {es_metrics['energy_cuts']}")
        print(f"  Background removed at 5 MeV: {es_metrics['background_removed'][-1]:.1f}%")
        print(f"  MARLEY retained at 5 MeV: {es_metrics['marley_retained'][-1]:.1f}%")
        print(f"  Main track retention at 5 MeV: {es_metrics['main_track_retention'][-1]:.1f}%")
    
    print("\n" + "="*60)


if __name__ == "__main__":
    # Load data
    data_file = 'energy_cut_scan_data.txt'
    if len(sys.argv) > 1:
        data_file = sys.argv[1]
    
    print(f"Loading data from {data_file}...")
    cc_data, es_data = load_data(data_file)
    
    print(f"Found {len(cc_data)} CC data points, {len(es_data)} ES data points")
    
    # Calculate metrics
    cc_metrics = calculate_metrics(cc_data)
    es_metrics = calculate_metrics(es_data)
    
    # Create plots
    plot_results(cc_metrics, es_metrics)
    
    # Create signal-to-background ratio plot
    cc_ratios, es_ratios = plot_signal_to_background(cc_data, es_data)
    
    # Print summary
    print_summary(cc_metrics, es_metrics)
    
    # Print S/B ratios at key energy cuts
    print("\nSignal-to-Background Ratios:")
    print("-" * 60)
    for i, ecut in enumerate(cc_ratios['energy_cuts']):
        if ecut in [0.0, 0.5, 1.0, 2.0, 5.0]:
            print(f"  {ecut:.1f} MeV - CC: {cc_ratios['signal_to_background'][i]:.3f}, "
                  f"ES: {es_ratios['signal_to_background'][i]:.3f}")
    
    print("\nDone!")
