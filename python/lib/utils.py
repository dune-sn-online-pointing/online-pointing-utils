"""
 * @file cluster_maker.py
 * @brief Reads takes tps array and makes clusters.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2024.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
"""

import numpy as np

# This function creates a channel map array, 
# where the first column is the channel number and the second column 
# is the plane view (0 for U, 1 for V, 2 for X).
# TODO understand if this can be avoided by reading from detchannelmaps
def create_channel_map_array(which_detector="APA"):
    '''
    :param which_detector, options are: 
     - APA for Horizontal Drift 
     - CRP for Vertical Drift (valid also for coldbox)
     - 50L for 50L detector, VD technology
    :return: channel map array
    '''
    # create the channel map array
    if which_detector == "APA":
        channel_map = np.empty((2560, 2), dtype=int)
        channel_map[:, 0] = np.linspace(0, 2559, 2560, dtype=int)
        channel_map[:, 1] = np.concatenate((np.zeros(800), np.ones(800), np.ones(960)*2))        
    elif which_detector == "CRP":
        channel_map = np.empty((3072, 2), dtype=int)
        channel_map[:, 0] = np.linspace(0, 3071, 3072, dtype=int)
        channel_map[:, 1] = np.concatenate((np.zeros(952), np.ones(952), np.ones(1168)*2))
    elif which_detector == "50L":
        channel_map = np.empty((128, 2), dtype=int)
        channel_map[:, 0] = np.linspace(0, 127, 128, dtype=int)
        # channel_map[:, 1] = np.concatenate(np.ones(49)*2, np.zeros(39), np.ones(40))
        channel_map[:, 1] = np.concatenate((np.zeros(39), np.ones(40), np.ones(49)*2))
    return channel_map

def distance_between_clusters(cluster1, cluster2):
    '''
    :param cluster1: first cluster
    :param cluster2: second cluster
    :return: distance between the two clusters
    '''
    pos1 = cluster1.get_reco_pos()
    pos2 = cluster2.get_reco_pos()
    return np.linalg.norm(pos1 - pos2)


def extract_basename(filepath):
    """
    Extract basename from tpstream file by removing _tpstream.root suffix.
    This allows consistent file tracking across pipeline stages.
    
    Example:
      prodmarley_..._20250826T091337Z_gen_000014_..._tpstream.root
      -> prodmarley_..._20250826T091337Z_gen_000014_...
    """
    from pathlib import Path
    filename = Path(filepath).name
    suffix = "_tpstream.root"
    if filename.endswith(suffix):
        return filename[:-len(suffix)]
    return filename


def find_files_matching_basenames(basenames, candidate_files):
    """
    Filter candidate files to only those matching the given basenames.
    This ensures skip/max from tpstream list applies to all pipeline stages.
    """
    matched_files = []
    for candidate in candidate_files:
        candidate_name = str(candidate) if hasattr(candidate, 'name') else str(candidate)
        from pathlib import Path
        filename = Path(candidate_name).name
        
        # Check if this file matches any of the basenames
        for basename in basenames:
            if basename in filename:
                matched_files.append(candidate)
                break
    
    return matched_files


def find_files_by_tpstream_basenames(json_config, folder, file_pattern, skip_files=0, max_files=None):
    """
    Find input files using tpstream file list as source of truth.
    
    This ensures skip/max parameters consistently refer to the same
    source files across all pipeline stages (backtrack, add_backgrounds,
    make_clusters, generate_images, create_volumes).
    
    Algorithm:
      1. Get list of tpstream files from signal_folder/tpstreams
      2. Apply skip/max to tpstream list
      3. Extract basenames from selected tpstream files
      4. Find files in target folder matching those basenames with the requested pattern
    
    Args:
        json_config: Dict with JSON configuration
        folder: Path to folder containing target files
        file_pattern: Pattern to match (e.g., "*_matched.root", "*_clusters.root")
        skip_files: Number of files to skip
        max_files: Maximum number of files to process (None = no limit)
    
    Returns:
        List of file paths matching basenames
    """
    from pathlib import Path
    
    # Get tpstream folder
    signal_folder = json_config.get('signal_folder', '')
    if not signal_folder:
        main_folder = json_config.get('main_folder', '')
        signal_folder = main_folder
    
    if not signal_folder:
        print("[find_files_by_tpstream_basenames] Warning: No signal_folder or main_folder found in JSON, falling back to simple glob")
        all_files = sorted(Path(folder).glob(file_pattern))
        if skip_files > 0:
            all_files = all_files[skip_files:]
        if max_files is not None:
            all_files = all_files[:max_files]
        return all_files
    
    tpstream_folder = Path(signal_folder) / "tpstreams"
    
    if not tpstream_folder.exists():
        print(f"[find_files_by_tpstream_basenames] Warning: Tpstream folder not found: {tpstream_folder}")
        print("[find_files_by_tpstream_basenames] Falling back to simple glob")
        all_files = sorted(Path(folder).glob(file_pattern))
        if skip_files > 0:
            all_files = all_files[skip_files:]
        if max_files is not None:
            all_files = all_files[:max_files]
        return all_files
    
    # Get tpstream files (the source of truth)
    tpstream_files = sorted(tpstream_folder.glob("*_tpstream.root"))
    
    if not tpstream_files:
        print(f"[find_files_by_tpstream_basenames] Warning: No tpstream files found in {tpstream_folder}")
        print("[find_files_by_tpstream_basenames] Falling back to simple glob")
        all_files = sorted(Path(folder).glob(file_pattern))
        if skip_files > 0:
            all_files = all_files[skip_files:]
        if max_files is not None:
            all_files = all_files[:max_files]
        return all_files
    
    # Apply skip and max to tpstream list
    if skip_files > 0 and skip_files < len(tpstream_files):
        tpstream_files = tpstream_files[skip_files:]
    if max_files is not None and max_files < len(tpstream_files):
        tpstream_files = tpstream_files[:max_files]
    
    # Extract basenames
    basenames = [extract_basename(str(f)) for f in tpstream_files]
    
    print(f"[find_files_by_tpstream_basenames] Using {len(basenames)} basenames from tpstream files (skip={skip_files}, max={max_files})")
    
    # Get all files matching the pattern
    all_pattern_files = sorted(Path(folder).glob(file_pattern))
    
    # Filter to only files matching our basenames
    matched_files = find_files_matching_basenames(basenames, all_pattern_files)
    
    print(f"[find_files_by_tpstream_basenames] Found {len(matched_files)} files matching basenames for pattern '{file_pattern}'")
    
    return matched_files


