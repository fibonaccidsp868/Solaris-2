@echo off
REM Windows: run as ADMINISTRATOR (needs CMake + Visual Studio with C++)
cmake -B build
cmake --build build --config Release
echo Done. Installed to C:\Program Files\Common Files\VST3 - rescan plugins in your DAW.
pause
