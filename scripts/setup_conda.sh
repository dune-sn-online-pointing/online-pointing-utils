# modified using Claude Code with Claude Opus 4.8 by H. Schellman on 2026-11-16 at 02:55:31 UTC
# Load conda's shell hook (idempotent; works in bash and zsh) so that
# `conda activate` succeeds whether this file is sourced into an interactive
# shell or run from a script. Then activate the ROOT environment.
for _c in "$HOME/miniconda3/etc/profile.d/conda.sh" \
          "$HOME/anaconda3/etc/profile.d/conda.sh" \
          "/opt/miniconda3/etc/profile.d/conda.sh" \
          "/opt/homebrew/Caskroom/miniconda/base/etc/profile.d/conda.sh"; do
    if [ -f "$_c" ]; then source "$_c"; break; fi
done
unset _c
conda activate cltest
