# Display Script Update Summary

## Changes Made

Added `--draw-mode` option to the `display.sh` bash script to allow users to easily switch between visualization modes from the command line.

## Modified File

**scripts/display.sh**

### Changes:

1. **Added help text** for the new option:
   ```bash
   --draw-mode <triangle|pentagon|rectangle>  Drawing mode (default: pentagon)
   ```

2. **Added variable initialization** with pentagon as default:
   ```bash
   drawMode="pentagon"  # default to pentagon
   ```

3. **Added command-line parsing** for the new option:
   ```bash
   --draw-mode)        drawMode="$2"; shift 2 ;;
   ```

4. **Added argument passing** to the cluster_display application:
   ```bash
   if [[ -n "$drawMode" ]]; then
     args+=( --draw-mode "$drawMode" )
   fi
   ```

## Usage Examples

### Pentagon Mode (Default)
```bash
./scripts/display.sh -j json/display/example.json
# or explicitly
./scripts/display.sh -j json/display/example.json --draw-mode pentagon
```

### Rectangle Mode
```bash
./scripts/display.sh -j json/display/example.json --draw-mode rectangle
```

### Triangle Mode
```bash
./scripts/display.sh -j json/display/example.json --draw-mode triangle
```

### With Input File
```bash
./scripts/display.sh \
  --input-clusters data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root \
  --draw-mode pentagon \
  --mode clusters
```

## Default Behavior

- **Default mode**: Pentagon (most accurate, with extended boundaries)
- If `--draw-mode` is not specified, pentagon mode is used
- Command-line `--draw-mode` overrides JSON settings if both are provided

## Integration with JSON

The draw mode can also be specified in JSON configuration files:

```json
{
  "clusters_file": "data/prod_cc/cc_valid_tick5_ch2_min2_tot1_clusters.root",
  "mode": "clusters",
  "draw_mode": "pentagon"
}
```

Command-line arguments take precedence over JSON settings.

## Complete Command Reference

```bash
./scripts/display.sh \
  --input-clusters <file.root> \      # Input file (or via JSON)
  -j <json_settings.json> \            # JSON settings (optional)
  --mode <clusters|events> \           # Display mode
  --draw-mode <triangle|pentagon|rectangle> \  # Drawing mode (NEW!)
  --only-marley \                      # Filter for MARLEY only (events mode)
  -v \                                 # Verbose mode
  --no-compile \                       # Skip recompilation
  --clean-compile                      # Clean and recompile
```

## Testing

Verified with help command:
```bash
$ ./scripts/display.sh --help
************************************************************************************************
Usage: ./scripts/display.sh --input-clusters <file.root> [-j <json_settings.json>] [options]
Options:
  -i|--input-clusters <file>     Input clusters ROOT file (required unless provided via JSON)
  -j|--json-settings <json>  Json file used as input. Relative path inside json/
  --mode <clusters|events>   Display mode (default: clusters)
  --draw-mode <triangle|pentagon|rectangle>  Drawing mode (default: pentagon)
  --only-marley              In events mode, show only MARLEY clusters
  -v|--verbose-mode          Turn on verbosity
  --no-compile               Do not recompile the code
  --clean-compile            Clean and recompile the code
  -h|--help                  Print this help message
************************************************************************************************
```

## Benefits

1. **User-friendly**: Easy to switch modes without editing code or JSON
2. **Flexible**: Works with both command-line and JSON configuration
3. **Consistent**: Same interface as other cluster_display options
4. **Documented**: Clear help text and default behavior
5. **Backward compatible**: Default to pentagon mode maintains expected behavior

## Files Created/Updated

1. ✅ **scripts/display.sh** - Added --draw-mode option
2. ✅ **docs/display_script_usage.md** - Comprehensive usage guide
3. ✅ **docs/display_modes_update.md** - Technical details on draw modes (from previous update)

## Related Documentation

- See `docs/display_modes_update.md` for technical details on pentagon/rectangle/triangle modes
- See `docs/display_script_usage.md` for usage examples and best practices
- See `docs/pentagon_algorithm_improvements.md` for pentagon algorithm details
