# Online Pointing Utils Documentation Index

This directory contains links and references to all documentation files in the online-pointing-utils repository.

## Documentation Hub

### Getting Started
- **[README.md](../README.md)** - Main repository documentation, overview, and getting started guide
- **[DEVELOPMENT.md](DEVELOPMENT.md)** - Development workflow, coding standards, and contribution guidelines
- **[CONFIGURATION.md](CONFIGURATION.md)** - Comprehensive configuration guide for all system components
- **[API_REFERENCE.md](API_REFERENCE.md)** - Quick reference to key APIs, data structures, and interfaces

## Main Documentation

- **[README.md](../README.md)** - Main repository documentation, overview, and getting started guide
- **[MARGIN_SCAN_ANALYSIS.md](../MARGIN_SCAN_ANALYSIS.md)** - Documentation for margin scan analysis procedures

## Component-Specific Documentation

### Parameters System
- **[PARAMETERS.md](../parameters/PARAMETERS.md)** - Parameter system documentation, configuration files format, and usage guide

### Scripts and Tools
- **[Scripts README](../scripts/README.md)** - Documentation for build scripts, execution workflows, and command-line tools

### Python Components
- **[TPs Plotting README](../python/tps_plotting/README.md)** - Python tools for trigger primitive plotting and visualization

### JSON Configurations
- **[JSON README](../json/README.md)** - Overview of JSON configuration files and their usage
- **[Cluster to Root README](../json/cluster_to_root/README.md)** - Configuration for cluster-to-ROOT conversion
- **[Clusters to Dataset README](../json/clusters_to_dataset/README.md)** - Configuration for cluster dataset generation
- **[Superimpose Signal and Backgrounds README](../json/superimpose_signal_and_backgrounds/README.md)** - Configuration for signal/background superimposition

## Submodule Documentation

The following external libraries are included as submodules with their own documentation:

- **[simple-cpp-logger](../submodules/simple-cpp-logger/README.md)** - Logging framework
- **[simple-cpp-cmd-line-parser](../submodules/simple-cpp-cmd-line-parser/README.md)** - Command-line argument parsing
- **[cpp-generic-toolbox](../submodules/cpp-generic-toolbox/README.md)** - Generic C++ utilities

## Quick Navigation

### For New Users
- Start with [README.md](../README.md) for basic usage
- Follow [CONFIGURATION.md](CONFIGURATION.md) for setup
- Check [Scripts README](../scripts/README.md) for available commands

### For Developers
- Review [DEVELOPMENT.md](DEVELOPMENT.md) for workflows and standards
- Check [API_REFERENCE.md](API_REFERENCE.md) for interfaces and data structures  
- Review [PARAMETERS.md](../parameters/PARAMETERS.md) for parameter system architecture
- See component-specific READMEs for detailed implementation notes

### For Analysis
- See [MARGIN_SCAN_ANALYSIS.md](../MARGIN_SCAN_ANALYSIS.md) for analysis workflows
- Review [JSON README](../json/README.md) for configuration options
- Check analysis-specific JSON configurations

### For System Integration
- Parameter configuration: [PARAMETERS.md](../parameters/PARAMETERS.md)
- Build and execution: [Scripts README](../scripts/README.md)
- Data processing workflows: [JSON README](../json/README.md)

## Repository Structure

```
online-pointing-utils/
├── README.md                          # Main documentation
├── MARGIN_SCAN_ANALYSIS.md           # Analysis procedures
├── docs/                             # Documentation hub
│   ├── README.md                     # This documentation index
│   ├── DEVELOPMENT.md                # Development guide
│   ├── CONFIGURATION.md              # Configuration guide
│   └── API_REFERENCE.md              # API reference
├── parameters/
│   └── PARAMETERS.md                 # Parameter system guide
├── scripts/
│   └── README.md                     # Build and execution scripts
├── python/tps_plotting/
│   └── README.md                     # Python plotting tools
├── json/
│   ├── README.md                     # JSON configurations overview
│   ├── cluster_to_root/README.md     # Cluster conversion configs
│   ├── clusters_to_dataset/README.md # Dataset generation configs
│   └── superimpose_signal_and_backgrounds/README.md
└── submodules/                       # External dependencies
    ├── simple-cpp-logger/README.md
    ├── simple-cpp-cmd-line-parser/README.md
    └── cpp-generic-toolbox/README.md
```

---

*This index is automatically maintained. If you add new documentation files, please update this index accordingly.*