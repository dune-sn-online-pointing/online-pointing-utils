#!/usr/bin/env python3
"""
Benchmark clustering performance on APA-split background data.

This script:
1. Takes background files (_bg_tps.root) as input
2. Splits TPG data by APA (4 APAs per file, 2560 channels each)
3. Runs clustering on each APA separately
4. Measures timing for each APA
5. Generates performance report with extrapolations to 10s and 100s

Author: GitHub Copilot
Date: 2025
"""

import argparse
import os
import sys
import time
import json
import subprocess
from pathlib import Path
import numpy as np

# APA configuration for 1x2x2 detector
CHANNELS_PER_APA = 2560
N_APAS = 4
TOTAL_CHANNELS = CHANNELS_PER_APA * N_APAS

# Default clustering parameters
DEFAULT_TICK_LIMIT = 5
DEFAULT_CHANNEL_LIMIT = 2
DEFAULT_MIN_TPS = 2
DEFAULT_ADC_TOTAL = 1
DEFAULT_ADC_ENERGY = 0

def split_file_by_apa(input_file, output_dir, split_by_apa_exe):
    """
    Split input file by APA using the split_by_apa executable.
    
    Args:
        input_file: Path to input ROOT file
        output_dir: Directory for APA-split output files
        split_by_apa_exe: Path to split_by_apa executable
    
    Returns:
        dict: {
            'success': bool,
            'runtime': float (seconds),
            'apa_files': list of paths to apa0_tps.root, apa1_tps.root, etc.
            'stderr': str
        }
    """
    start_time = time.time()
    
    try:
        result = subprocess.run(
            [split_by_apa_exe, input_file, output_dir],
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout for splitting
        )
        
        end_time = time.time()
        runtime = end_time - start_time
        
        # Check for output files
        apa_files = [os.path.join(output_dir, f"apa{i}_tps.root") for i in range(N_APAS)]
        all_exist = all(os.path.exists(f) for f in apa_files)
        
        return {
            'success': result.returncode == 0 and all_exist,
            'runtime': runtime,
            'apa_files': apa_files if all_exist else [],
            'stdout': result.stdout,
            'stderr': result.stderr,
            'returncode': result.returncode
        }
        
    except subprocess.TimeoutExpired:
        end_time = time.time()
        return {
            'success': False,
            'runtime': end_time - start_time,
            'apa_files': [],
            'stdout': '',
            'stderr': 'TIMEOUT after 300 seconds',
            'returncode': -1
        }
    except Exception as e:
        end_time = time.time()
        return {
            'success': False,
            'runtime': end_time - start_time,
            'apa_files': [],
            'stdout': '',
            'stderr': str(e),
            'returncode': -1
        }


def create_apa_json_config(input_file, output_dir, apa_id, tick_limit, channel_limit, min_tps, adc_total, adc_energy):
    """
    Create JSON configuration file for clustering a specific APA.
    
    Args:
        input_file: Path to APA-specific input ROOT file
        output_dir: Directory for output
        apa_id: APA number (0-3)
        tick_limit: Tick clustering parameter
        channel_limit: Channel clustering parameter
        min_tps: Minimum TPs per cluster
        adc_total: ADC total threshold
        adc_energy: ADC energy threshold
    
    Returns:
        Path to created JSON config file
    """
    # Create APA-specific subdirectory for the input file to avoid find_input_files 
    # picking up all APA files when only one is needed
    apa_input_dir = os.path.join(os.path.dirname(input_file), f"apa{apa_id}")
    os.makedirs(apa_input_dir, exist_ok=True)
    
    # Copy or symlink the APA file to its own directory
    apa_input_file = os.path.join(apa_input_dir, os.path.basename(input_file))
    if not os.path.exists(apa_input_file):
        os.symlink(input_file, apa_input_file)
    
    config = {
        "tps_folder": apa_input_dir,  # APA-specific directory
        "tps_files": [os.path.basename(input_file)],  # Just the filename
        "outputFolder": output_dir,
        "outputFilename": f"apa{apa_id}_clusters",
        "tick_limit": tick_limit,
        "channel_limit": channel_limit,
        "min_tps_to_cluster": min_tps,
        "tot_cut": adc_total,
        "energy_cut": adc_energy
    }
    
    json_filename = os.path.join(output_dir, f"apa{apa_id}_config.json")
    with open(json_filename, 'w') as f:
        json.dump(config, f, indent=2)
    
    return json_filename


def run_clustering(make_clusters_exe, json_config):
    """
    Run the make_clusters executable with given JSON configuration.
    
    Args:
        make_clusters_exe: Path to make_clusters executable
        json_config: Path to JSON configuration file
    
    Returns:
        dict: {
            'success': bool,
            'runtime': float (seconds),
            'stdout': str,
            'stderr': str
        }
    """
    start_time = time.time()
    
    try:
        result = subprocess.run(
            [make_clusters_exe, '-j', json_config],
            capture_output=True,
            text=True,
            timeout=600  # 10 minute timeout
        )
        
        end_time = time.time()
        runtime = end_time - start_time
        
        return {
            'success': result.returncode == 0,
            'runtime': runtime,
            'stdout': result.stdout,
            'stderr': result.stderr,
            'returncode': result.returncode
        }
        
    except subprocess.TimeoutExpired:
        end_time = time.time()
        return {
            'success': False,
            'runtime': end_time - start_time,
            'stdout': '',
            'stderr': 'TIMEOUT after 600 seconds',
            'returncode': -1
        }
    except Exception as e:
        end_time = time.time()
        return {
            'success': False,
            'runtime': end_time - start_time,
            'stdout': '',
            'stderr': str(e),
            'returncode': -1
        }


def parse_clustering_output(stdout):
    """
    Parse make_clusters output to extract event count and other metrics.
    
    Args:
        stdout: Standard output from make_clusters
    
    Returns:
        dict: Parsed metrics including n_events, n_clusters, etc.
    """
    metrics = {
        'n_events': 0,
        'n_clusters': 0,
        'n_tps': 0
    }
    
    for line in stdout.split('\n'):
        # Look for event count indicators
        if 'event' in line.lower() and any(char.isdigit() for char in line):
            # Try to extract number
            words = line.split()
            for i, word in enumerate(words):
                if 'event' in word.lower() and i + 1 < len(words):
                    try:
                        metrics['n_events'] = int(words[i + 1])
                    except ValueError:
                        pass
        
        # Look for cluster count
        if 'cluster' in line.lower() and any(char.isdigit() for char in line):
            words = line.split()
            for i, word in enumerate(words):
                if 'cluster' in word.lower() and i + 1 < len(words):
                    try:
                        metrics['n_clusters'] = int(words[i + 1])
                    except ValueError:
                        pass
    
    return metrics


def benchmark_file(input_file, output_dir, make_clusters_exe, split_by_apa_exe, params):
    """
    Benchmark clustering on all APAs for one input file.
    
    Args:
        input_file: Path to background ROOT file
        output_dir: Output directory
        make_clusters_exe: Path to make_clusters executable
        split_by_apa_exe: Path to split_by_apa executable
        params: Dict of clustering parameters
    
    Returns:
        dict: Benchmark results
    """
    filename = os.path.basename(input_file)
    file_results = {
        'filename': filename,
        'input_path': input_file,
        'split_runtime': 0.0,
        'apa_results': [],
        'total_clustering_runtime': 0.0,
        'success': True
    }
    
    print(f"\n{'='*80}")
    print(f"Benchmarking file: {filename}")
    print(f"{'='*80}")
    
    # Step 1: Split by APA (NOT timed for clustering benchmark)
    print(f"\n  Splitting by APA... ", end='', flush=True)
    split_dir = os.path.join(output_dir, "split")
    os.makedirs(split_dir, exist_ok=True)
    
    split_result = split_file_by_apa(input_file, split_dir, split_by_apa_exe)
    file_results['split_runtime'] = split_result['runtime']
    
    if not split_result['success']:
        print(f"FAILED ({split_result['runtime']:.2f}s)")
        if split_result['stderr']:
            print(f"    Error: {split_result['stderr'][:200]}")
        file_results['success'] = False
        return file_results
    
    print(f"OK ({split_result['runtime']:.2f}s)")
    
    # Step 2: Cluster each APA separately (THIS is what we time!)
    for apa_id in range(N_APAS):
        print(f"\n  APA {apa_id}: ", end='', flush=True)
        
        # Create APA-specific output directory
        apa_output_dir = os.path.join(output_dir, f"apa{apa_id}")
        os.makedirs(apa_output_dir, exist_ok=True)
        
        # Use the pre-split APA file
        apa_input_file = split_result['apa_files'][apa_id]
        
        # Create JSON config for this APA
        json_config = create_apa_json_config(
            apa_input_file, apa_output_dir, apa_id,
            params['tick_limit'], params['channel_limit'],
            params['min_tps'], params['adc_total'], params['adc_energy']
        )
        
        # Run clustering (ONLY THIS IS TIMED!)
        result = run_clustering(make_clusters_exe, json_config)
        
        # Parse output
        metrics = parse_clustering_output(result['stdout'])
        
        # Store results
        apa_result = {
            'apa_id': apa_id,
            'runtime': result['runtime'],
            'success': result['success'],
            'returncode': result['returncode'],
            'metrics': metrics,
            'json_config': json_config,
            'input_file': apa_input_file
        }
        
        file_results['apa_results'].append(apa_result)
        file_results['total_clustering_runtime'] += result['runtime']
        
        if not result['success']:
            file_results['success'] = False
            print(f"FAILED (code {result['returncode']}, {result['runtime']:.2f}s)")
            if result['stderr']:
                print(f"    Error: {result['stderr'][:200]}")
        else:
            print(f"OK ({result['runtime']:.2f}s, {metrics['n_events']} events)")
    
    return file_results


def generate_report(all_results, output_path, params):
    """
    Generate markdown report with benchmark results and extrapolations.
    
    Args:
        all_results: List of file benchmark results
        output_path: Path to save markdown report
        params: Clustering parameters used
    """
    with open(output_path, 'w') as f:
        f.write("# Clustering Performance Benchmark Report\n\n")
        f.write(f"**Generated:** {time.strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        
        f.write("## Configuration\n\n")
        f.write("### Clustering Parameters\n\n")
        f.write(f"- **Tick Limit:** {params['tick_limit']}\n")
        f.write(f"- **Channel Limit:** {params['channel_limit']}\n")
        f.write(f"- **Min TPs per Cluster:** {params['min_tps']}\n")
        f.write(f"- **ADC Total Cut:** {params['adc_total']}\n")
        f.write(f"- **ADC Energy Cut:** {params['adc_energy']}\n\n")
        
        f.write("### Detector Configuration\n\n")
        f.write(f"- **APAs per Detector:** {N_APAS}\n")
        f.write(f"- **Channels per APA:** {CHANNELS_PER_APA}\n")
        f.write(f"- **Total Channels:** {TOTAL_CHANNELS}\n\n")
        
        f.write("## Summary Statistics\n\n")
        
        # Calculate aggregate statistics
        total_files = len(all_results)
        successful_files = sum(1 for r in all_results if r['success'])
        total_split_time = sum(r['split_runtime'] for r in all_results)
        
        all_runtimes = []
        all_events = []
        apa_runtimes = {i: [] for i in range(N_APAS)}
        apa_events = {i: [] for i in range(N_APAS)}
        
        for file_result in all_results:
            if file_result['success']:
                for apa_result in file_result['apa_results']:
                    if apa_result['success']:
                        all_runtimes.append(apa_result['runtime'])
                        all_events.append(apa_result['metrics']['n_events'])
                        
                        apa_id = apa_result['apa_id']
                        apa_runtimes[apa_id].append(apa_result['runtime'])
                        apa_events[apa_id].append(apa_result['metrics']['n_events'])
        
        f.write(f"- **Files Processed:** {total_files}\n")
        f.write(f"- **Successful:** {successful_files}\n")
        f.write(f"- **Failed:** {total_files - successful_files}\n")
        f.write(f"- **Total APA Splitting Time:** {total_split_time:.2f} seconds (NOT included in clustering benchmarks)\n\n")
        
        if all_runtimes:
            avg_runtime = np.mean(all_runtimes)
            std_runtime = np.std(all_runtimes)
            avg_events = np.mean(all_events)
            
            f.write("### Timing Statistics (per APA)\n\n")
            f.write(f"- **Average Runtime:** {avg_runtime:.3f} ± {std_runtime:.3f} seconds\n")
            f.write(f"- **Min Runtime:** {min(all_runtimes):.3f} seconds\n")
            f.write(f"- **Max Runtime:** {max(all_runtimes):.3f} seconds\n")
            f.write(f"- **Average Events per Run:** {avg_events:.1f}\n")
            
            # Calculate per-event timing
            if avg_events > 0:
                time_per_event = avg_runtime / avg_events * 1000  # Convert to ms
                f.write(f"- **Average Time per Event:** {time_per_event:.2f} ms\n\n")
                
                # Extrapolation calculations
                f.write("## Performance Extrapolations\n\n")
                
                # Assuming ~3ms per event (user's estimate), calculate how many events in 10s and 100s
                event_duration_ms = 3.0  # ms, from detector simulation
                
                events_in_10s = (10 * 1000) / event_duration_ms  # 10 seconds worth of events
                events_in_100s = (100 * 1000) / event_duration_ms  # 100 seconds worth of events
                
                f.write(f"### Assumptions\n\n")
                f.write(f"- **Event Duration:** {event_duration_ms:.1f} ms (from simulation)\n")
                f.write(f"- **Events in 10s:** {events_in_10s:.0f} events\n")
                f.write(f"- **Events in 100s:** {events_in_100s:.0f} events\n\n")
                
                # Clustering time for 10s and 100s windows
                clustering_time_10s_per_apa = (events_in_10s / avg_events) * avg_runtime
                clustering_time_100s_per_apa = (events_in_100s / avg_events) * avg_runtime
                
                f.write(f"### Clustering Time per APA\n\n")
                f.write(f"- **10s window:** {clustering_time_10s_per_apa:.2f} seconds ({clustering_time_10s_per_apa/60:.2f} minutes)\n")
                f.write(f"- **100s window:** {clustering_time_100s_per_apa:.2f} seconds ({clustering_time_100s_per_apa/60:.2f} minutes)\n\n")
                
                # Total time for all APAs
                clustering_time_10s_total = clustering_time_10s_per_apa * N_APAS
                clustering_time_100s_total = clustering_time_100s_per_apa * N_APAS
                
                f.write(f"### Total Clustering Time (all {N_APAS} APAs)\n\n")
                f.write(f"- **10s window:** {clustering_time_10s_total:.2f} seconds ({clustering_time_10s_total/60:.2f} minutes)\n")
                f.write(f"- **100s window:** {clustering_time_100s_total:.2f} seconds ({clustering_time_100s_total/60:.2f} minutes)\n\n")
                
                # Throughput metrics
                throughput_10s = 10.0 / clustering_time_10s_per_apa  # Real-time factor
                throughput_100s = 100.0 / clustering_time_100s_per_apa
                
                f.write(f"### Throughput (per APA)\n\n")
                f.write(f"- **10s window:** {throughput_10s:.2f}x real-time\n")
                f.write(f"- **100s window:** {throughput_100s:.2f}x real-time\n\n")
        
        # Per-APA breakdown
        f.write("## Per-APA Performance\n\n")
        for apa_id in range(N_APAS):
            if apa_runtimes[apa_id]:
                avg_apa_runtime = np.mean(apa_runtimes[apa_id])
                std_apa_runtime = np.std(apa_runtimes[apa_id])
                avg_apa_events = np.mean(apa_events[apa_id])
                
                f.write(f"### APA {apa_id}\n\n")
                f.write(f"- **Runs:** {len(apa_runtimes[apa_id])}\n")
                f.write(f"- **Average Runtime:** {avg_apa_runtime:.3f} ± {std_apa_runtime:.3f} seconds\n")
                f.write(f"- **Average Events:** {avg_apa_events:.1f}\n")
                if avg_apa_events > 0:
                    time_per_event = avg_apa_runtime / avg_apa_events * 1000
                    f.write(f"- **Time per Event:** {time_per_event:.2f} ms\n")
                f.write("\n")
        
        # Detailed results table
        f.write("## Detailed Results\n\n")
        f.write("**Note:** Split time is the time to separate TPs by APA (not included in clustering benchmarks).\n\n")
        f.write("| File | Split (s) | APA | Clustering (s) | Events | Clusters | Time/Event (ms) | Status |\n")
        f.write("|------|-----------|-----|----------------|--------|----------|-----------------|--------|\n")
        
        for file_result in all_results:
            filename_short = file_result['filename'][:40] + "..." if len(file_result['filename']) > 40 else file_result['filename']
            split_time = file_result['split_runtime']
            
            for apa_result in file_result['apa_results']:
                runtime = apa_result['runtime']
                n_events = apa_result['metrics']['n_events']
                n_clusters = apa_result['metrics']['n_clusters']
                time_per_event = (runtime / n_events * 1000) if n_events > 0 else 0
                status = "✓" if apa_result['success'] else "✗"
                
                f.write(f"| {filename_short} | {split_time:.2f} | {apa_result['apa_id']} | {runtime:.2f} | {n_events} | {n_clusters} | {time_per_event:.2f} | {status} |\n")
        
        # Instructions for reproduction
        f.write("\n## Reproduction Instructions\n\n")
        f.write("To reproduce these benchmarks on another machine:\n\n")
        f.write("```bash\n")
        f.write("# 1. Build the project\n")
        f.write("cd /path/to/online-pointing-utils\n")
        f.write("mkdir -p build && cd build\n")
        f.write("cmake ..\n")
        f.write("make\n\n")
        f.write("# 2. Run the benchmark script\n")
        f.write("cd ..\n")
        f.write("python scripts/benchmark_clustering.py \\\n")
        f.write("  --input-files data/prod_cc/bkgs/*_bg_tps.root \\\n")
        f.write("  --output-dir benchmark_results \\\n")
        f.write("  --n-files 5 \\\n")
        f.write(f"  --tick-limit {params['tick_limit']} \\\n")
        f.write(f"  --channel-limit {params['channel_limit']} \\\n")
        f.write(f"  --min-tps {params['min_tps']}\n")
        f.write("```\n\n")
        
        f.write("### System Information\n\n")
        f.write("Record your system specifications:\n")
        f.write("- CPU model and cores\n")
        f.write("- RAM capacity\n")
        f.write("- Operating system\n")
        f.write("- Compiler version\n\n")
    
    print(f"\n{'='*80}")
    print(f"Report saved to: {output_path}")
    print(f"{'='*80}\n")


def main():
    parser = argparse.ArgumentParser(
        description='Benchmark clustering performance on APA-split background data',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument('-i', '--input-files', nargs='+', required=True,
                        help='Input background ROOT files (*_bg_tps.root)')
    parser.add_argument('-o', '--output-dir', default='benchmark_results',
                        help='Output directory for results and reports')
    parser.add_argument('-e', '--make-clusters-exe', 
                        default='./build/src/app/make_clusters',
                        help='Path to make_clusters executable')
    parser.add_argument('-s', '--split-by-apa-exe',
                        default='./build/src/app/split_by_apa',
                        help='Path to split_by_apa executable')
    parser.add_argument('-n', '--n-files', type=int, default=None,
                        help='Number of files to process (default: all)')
    parser.add_argument('-t', '--tick-limit', type=int, default=DEFAULT_TICK_LIMIT,
                        help=f'Tick clustering limit (default: {DEFAULT_TICK_LIMIT})')
    parser.add_argument('-c', '--channel-limit', type=int, default=DEFAULT_CHANNEL_LIMIT,
                        help=f'Channel clustering limit (default: {DEFAULT_CHANNEL_LIMIT})')
    parser.add_argument('-m', '--min-tps', type=int, default=DEFAULT_MIN_TPS,
                        help=f'Minimum TPs per cluster (default: {DEFAULT_MIN_TPS})')
    parser.add_argument('--adc-total', type=int, default=DEFAULT_ADC_TOTAL,
                        help=f'ADC total threshold (default: {DEFAULT_ADC_TOTAL})')
    parser.add_argument('--adc-energy', type=int, default=DEFAULT_ADC_ENERGY,
                        help=f'ADC energy threshold (default: {DEFAULT_ADC_ENERGY})')
    parser.add_argument('--report-name', default='CLUSTERING_BENCHMARK_REPORT.md',
                        help='Output report filename')
    
    args = parser.parse_args()
    
    # Validate inputs
    if not os.path.exists(args.make_clusters_exe):
        print(f"Error: make_clusters executable not found: {args.make_clusters_exe}")
        print("Please build the project first or specify correct path with -e")
        sys.exit(1)
    
    if not os.path.exists(args.split_by_apa_exe):
        print(f"Error: split_by_apa executable not found: {args.split_by_apa_exe}")
        print("Please build the project first or specify correct path with -s")
        sys.exit(1)
    
    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)
    
    # Limit number of files if requested
    input_files = args.input_files[:args.n_files] if args.n_files else args.input_files
    
    print(f"\n{'='*80}")
    print("CLUSTERING PERFORMANCE BENCHMARK")
    print(f"{'='*80}")
    print(f"Files to process: {len(input_files)}")
    print(f"Output directory: {args.output_dir}")
    print(f"Make clusters: {args.make_clusters_exe}")
    print(f"Split by APA: {args.split_by_apa_exe}")
    print(f"{'='*80}\n")
    
    # Clustering parameters
    params = {
        'tick_limit': args.tick_limit,
        'channel_limit': args.channel_limit,
        'min_tps': args.min_tps,
        'adc_total': args.adc_total,
        'adc_energy': args.adc_energy
    }
    
    # Run benchmarks
    all_results = []
    for input_file in input_files:
        if not os.path.exists(input_file):
            print(f"Warning: File not found: {input_file}")
            continue
        
        result = benchmark_file(input_file, args.output_dir, args.make_clusters_exe, args.split_by_apa_exe, params)
        all_results.append(result)
    
    # Generate report
    report_path = os.path.join(args.output_dir, args.report_name)
    generate_report(all_results, report_path, params)
    
    # Save raw results as JSON
    json_path = os.path.join(args.output_dir, 'benchmark_results.json')
    with open(json_path, 'w') as f:
        json.dump(all_results, f, indent=2)
    print(f"Raw results saved to: {json_path}\n")


if __name__ == '__main__':
    main()
