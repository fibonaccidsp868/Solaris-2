#!/usr/bin/env bash
# macOS / Linux: builds Solaris 2 and installs the VST3 (~/Library/Audio/Plug-Ins/VST3 or ~/.vst3)
set -e
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
echo "Done. Rescan plugins in your DAW."
