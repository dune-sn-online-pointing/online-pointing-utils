# Development Guide

This guide covers development workflows, coding standards, and contribution guidelines for the online-pointing-utils project.

## Quick Links to Key Documentation

- **[Main README](../README.md)** - Project overview and basic usage
- **Parameter System (`parameters/*.dat`)** - Configuration and parameter management
- **[Build Scripts](../scripts/README.md)** - Compilation and execution workflows
- **[Analysis Documentation](../MARGIN_SCAN_ANALYSIS.md)** - Analysis procedures and workflows

## Development Workflow

### Setting Up the Environment
1. Follow the setup instructions in [README.md](../README.md)
2. Configure parameters in `parameters/*.dat`
3. Build the project using scripts documented in [scripts/README.md](../scripts/README.md)

### Code Organization

The project is organized into several key libraries:
- **globalLib** - Core utilities and global constants
- **objectsLibs** - Data structures (Cluster, TriggerPrimitive, etc.)
- **backtrackingLibs** - Position calculation and backtracking algorithms
- **clusteringLibs** - Clustering algorithms and utilities
- **planesMatchingLibs** - Multi-plane cluster matching

### Parameter System

The project uses a flexible parameter system based on `parameters/*.dat`. Key points:
- Parameters are stored in `.dat` files with structured format
- Runtime configuration without recompilation
- Environment variable support via `PARAMETERS_DIR`
- Backward compatibility with legacy code

### Configuration Management

JSON configuration files are documented in [../json/README.md](../json/README.md) and provide:
- Algorithm tuning parameters
- Input/output file specifications
- Processing workflows

## Testing and Validation

### Running Tests
```bash
# Compile the project
./scripts/compile.sh

# Run analysis workflows
./scripts/analyze_tps.sh -j json/analyze_tps/example.json
./scripts/cluster.sh -j json/cluster/example.json
```

### Adding New Components

1. **Parameters**: Add to appropriate `.dat` file in `parameters/`
2. **Algorithms**: Implement in relevant library (clustering, objects, etc.)
3. **Applications**: Add to `src/app/` with corresponding script in `scripts/`
4. **Configuration**: Add JSON examples in `json/`

### Documentation Standards

- Update relevant README files when adding features
- Include parameter descriptions in `.dat` files
- Add usage examples for new scripts
- Update this development guide for workflow changes

## Troubleshooting

### Common Issues
1. **Parameter loading errors**: Check `PARAMETERS_DIR` environment variable
2. **Compilation errors**: Ensure all dependencies are properly linked
3. **Runtime errors**: Verify JSON configuration file paths and content

### Getting Help
- Check component-specific README files first
- Review parameter configuration in `parameters/*.dat`
- Look at example configurations in `json/` directories