# Hidra Miscellaneous Tools

This directory contains utility scripts and tools for the Hidra DAQ system.

## Structure

### `parse_eudaq_events/`
**EUDAQ Event File Parser** - Tools for parsing and analyzing binary EUDAQ event files

- **`parse_eudaq_events.py`** - Main parser script (executable)
  - Interactive event viewer (like `less`)
  - Batch mode for scripting
  - Statistics and analysis
  - Hex dump display

- **`README.md`** - Complete documentation and usage guide
- **`examples.sh`** - Runnable examples of all parser features  
- **`DELIVERY_SUMMARY.md`** - Technical delivery documentation

**Quick Start:**
```bash
cd /home/rscaglio/work/eudaq_hidra/user/hidra/

# Show file statistics
./misc/parse_eudaq_events/parse_eudaq_events.py run/out_data/run024_260422162755.raw -s

# Interactive viewer
./misc/parse_eudaq_events/parse_eudaq_events.py run/out_data/run024_260422162755.raw

# View examples
bash ./misc/parse_eudaq_events/examples.sh
```

### Other Files in This Directory
- `setup.sh` - Environment setup script
- `hexdump_fers2025.py` - FERS data analysis tool
- `CMakePresets.hidra.json` - CMake configuration presets
- `README.md` - General documentation

## Related Tools

Other detector-specific tools are located in their respective subfolders:
- `../fers/misc/` - FERS detector configuration files and run scripts
- `../xdc/` - XDC (QTP) detector tools
- `../dry/` - Dry run (simulator) tools

## Adding New Tools

When adding new utility scripts or tools to this directory:
1. Create a dedicated subfolder if it's a multi-file tool suite
2. Include a README explaining purpose and usage
3. Keep related files together
4. Update this INDEX.md document

## Documentation

For detailed documentation on any tool, refer to the README in its respective folder.
