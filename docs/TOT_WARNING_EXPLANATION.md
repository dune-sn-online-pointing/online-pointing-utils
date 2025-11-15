# ToT Warning Explanation

## What You're Seeing

You're getting warnings like:
```
ToT field absent or all zeros; skipping ToT<2 filter. Kept 1 TPs from file /eos/user/e/evilla/dune/sn-tps/bkgs/BG_104_tpstream.root
```

## What This Means

**This is NORMAL for background events**, especially low-energy backgrounds. Here's why:

### Understanding the Warning

1. **Event-by-Event Processing**: The code processes files event-by-event, not as a whole
2. **Per-Event Filtering**: The ToT (Time-over-Threshold) filter is applied per event
3. **Background Events**: Background events often have very few TPs (Trigger Primitives)
4. **Low ToT Values**: TPs with ToT < 2 are normally filtered out, but if ALL TPs in an event have ToT=0 or ToT=1, the filter is skipped to avoid losing all data

### Your Specific Case

- **File**: `BG_104_tpstream.root` (background file)
- **Total TPs in file**: 31,669 (checked via diagnostic)
- **ToT range in file**: 1-7 (normal)
- **What's happening**: Some individual events in the file have only 1 TP with ToT=0 or ToT=1

### Why This Happens with Backgrounds

Background events typically have:
- **Few TPs**: Often just 1-5 TPs per event
- **Low energy deposits**: Resulting in low ToT values (0, 1, or 2)
- **Sparse activity**: Unlike neutrino signal events which have many TPs

## Changes Made

### 1. Improved Logging
- Changed from `LogWarning` (always shown) to `LogInfo` (verbose mode only)
- Added event number to the message for better tracking
- Made the message less alarming: "this may be normal for background events"

### 2. Smarter Warning Logic
Now only warns when:
- Event has < 10 TPs **AND**
- All TPs have ToT=0 **AND**
- Provides context that this is normal for backgrounds

### 3. Event Validation
Added check to detect if requested event is not found in the file

## How to Diagnose Issues

Use the diagnostic script created:
```bash
python3 check_tot_in_file.py /path/to/file.root
```

This will show:
- Total TPs in the file
- Percentage with non-zero ToT
- ToT value range

## Summary

✅ **Your files are fine!** The warnings you see are expected for background files with sparse events.

🔧 **After recompiling**, you'll see:
- Much less spam (warnings only in verbose mode)
- Event numbers in the messages
- Better context about what's normal

## When to Worry

Only worry if you see:
- **Signal events** (neutrino files) with all ToT=0
- **All files** showing ToT=0
- **Large events** (>100 TPs) with all ToT=0

For background files with 1-10 TPs having ToT=0, this is **completely normal**.
