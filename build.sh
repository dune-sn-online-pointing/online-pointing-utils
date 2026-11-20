#!/bin/bash
# modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-15 at 21:27:35 UTC
#
# Convenience wrapper: set up the environment and build the project.
#   ./build.sh                               # incremental build of build/full
#   ./build.sh --target read_trigger_trees   # build one target
#   ./build.sh -j8                            # parallel build
# Any extra arguments are passed straight through to `cmake --build`.
# (Temporary helper; to be folded into the project CMake setup later.)

HERE="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"

# Make `conda` available in this (possibly non-interactive) shell so that
# scripts/init.sh -> setup_conda.sh can activate the cltest environment.
if ! command -v conda >/dev/null 2>&1; then
    for c in "$HOME/miniconda3/etc/profile.d/conda.sh" \
             "$HOME/anaconda3/etc/profile.d/conda.sh"; do
        [ -f "$c" ] && source "$c" && break
    done
fi

# Set up ROOT / conda environment (puts root-config on PATH, etc.).
source "$HERE/scripts/init.sh"

# Build. (Plain call, not exec: exec would replace the caller's shell, which
# kills the terminal if this script is sourced. The script's exit status is
# cmake's either way, since this is the last command.)
cmake --build "$HERE/build" "$@"
