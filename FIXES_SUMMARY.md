# Summary of Fixes

## Issues Fixed

### 1. SimIDE Matching Bug (CRITICAL)
**Problem**: `match_tps_to_simides_direct` was called with wrong event number
- Was using: `event_number` (function parameter)
- Should use: `this_event_number` (actual event from file)
- **Impact**: SimIDEs not being matched, warnings about "no SimIDEs found"

**Fix**: Changed line 458 to use `this_event_number`

### 2. Events Without TPs (Background Files)
**Problem**: Warning spam when events have MC truth but no TPs
- This is NORMAL for background files
- Some events have particles but no TPs (filtered by ToT, etc.)
- Was showing as `LogWarning` always

**Fix**: 
- Changed to `LogInfo` in verbose mode only
- Added context: "has no TPs (skipping)"
- Early return prevents downstream errors

### 3. ToT Filter Warnings
**Problem**: Too many warnings about ToT=0 in background files
- Background events naturally have few TPs with low ToT
- Was warning for every event with ToT=0

**Fix**:
- Changed from `LogWarning` to `LogInfo` (verbose mode only)
- Only warn if < 10 TPs AND all have ToT=0
- Added event number to messages
- Added context: "may be normal for background events"

## Understanding Event Processing

### Event Discovery Flow
```
backtrack_tpstream.cpp:
  1. Opens file
  2. Reads mctruth tree to find which events exist
  3. For each discovered event number (iEvent):
     - Calls read_tpstream(filename, ..., iEvent, ...)
```

### read_tpstream Flow
```
Backtracking.cpp::read_tpstream(event_number):
  - event_number = the event to find (from mctruth discovery)
  - this_event_number = temporary variable to read Event branch
  
  For each tree (TPs, MCparticles, MCtruth, SimIDEs):
    1. Set branch address to read Event values
    2. Call get_first_and_last_event(..., &this_event_number, event_number, ...)
       - Searches tree for entries where Event == event_number
       - Returns first/last entry indices
    3. Process entries in that range
```

### Why Events Can Be Missing
- **MC truth** may have events with no TPs (e.g., particles outside detector)
- **TPs tree** may have events with no MC truth (detector noise)
- **ToT filtering** removes TPs with ToT < 2, can eliminate entire events
- This is **normal and expected** for background files

## Files Changed

### src/backtracking/Backtracking.cpp
1. Line 52-61: Added early return for events without TPs
2. Line 122: Uses `this_event_number` for TP.SetEvent()
3. Line 142-148: Improved ToT warning messages
4. Line 458: Fixed SimIDE matching to use `this_event_number`

## Testing
After recompiling, you should see:
- ✅ Much cleaner output (less warning spam)
- ✅ SimIDEs correctly matched to TPs
- ✅ Events without TPs silently skipped (or logged in verbose mode)
- ✅ Better diagnostic messages with event numbers

## When to Worry
Only worry if you see:
- **Signal files** (neutrino) with all events having no TPs
- **All files** showing no SimIDEs matches
- **Errors** (LogError) rather than Info/Warning messages
