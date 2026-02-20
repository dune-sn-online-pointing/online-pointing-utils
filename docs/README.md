# Online Pointing Utils

This repository provides C++ and Python utilities used in the DUNE supernova online pointing workflow: backtracking TPStreams, adding backgrounds, clustering, 3-plane matching, and producing “volume” images around reconstructed tracks.

## Build

Prerequisites:
- CMake + a C++17 compiler
- ROOT

Build (in-source `build/` is already used in this repo):
```bash
cd build
cmake ..
make -j
```

Executables are produced under `build/src/app/`.

## Run (typical workflow)

The intended entrypoint is the pipeline wrapper:
```bash
./scripts/sequence.sh -s <sample> -j <settings.json> --all
```

Settings discovery is handled by `scripts/findSettings.sh`. In practice, the easiest option is to keep your settings JSON under `json/` and pass either the basename or a `json/...` path.

## Documentation

- [APPS.md](APPS.md): what each app/script does and how they fit together
- [CONFIGURATION.md](CONFIGURATION.md): configuration layers (parameters + JSON + env)
- [json/README.md](json/README.md): JSON settings conventions
- [parameters/PARAMETERS.md](parameters/PARAMETERS.md): parameter files and keys
- [VOLUME_IMAGES.md](VOLUME_IMAGES.md): volume image format and conventions
- [code-structure.md](code-structure.md): code layout and architecture notes

Part of the DUNE DAQ Application Framework. See COPYING for licensing details.

## Contact

DUNE Supernova Online Pointing Group
- Repository: [dune-sn-online-pointing/online-pointing-utils](https://github.com/dune-sn-online-pointing/online-pointing-utils)
- DUNE Collaboration: [dunescience.org](https://www.dunescience.org/)

---

**Last Updated**: November 2025  
**Branch**: evilla/direct-tp-simide-matching
