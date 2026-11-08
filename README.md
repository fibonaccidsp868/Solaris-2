# Solaris 2 - Harmonic Saturator (VST3) by Fibonacci

## Easiest install (Windows)
1. Upload this whole folder to your GitHub repo (keep `Source/` and `.github/`), replacing the old files.
2. Open the **Actions** tab -> "Build Solaris 2" -> wait for the green tick.
3. Download **Solaris2-Windows-Installer** -> unzip -> run `Solaris2-Setup.exe`.
   (It installs to `C:\Program Files\Common Files\VST3`. Rescan plugins in your DAW.)
No installer? Download `Solaris2-VST3-windows-latest` and copy the `Solaris 2.vst3` folder there yourself.
macOS: copy to `~/Library/Audio/Plug-Ins/VST3`. Linux: `~/.vst3`.

## Build locally
`build.bat` (Windows, Administrator) or `./build.sh`. JUCE 8.0.6 is downloaded automatically.

## What is inside
* 4x oversampled (linear-phase FIR), double-precision filters, zero added noise.
* **Prism engine**: erf saturation curve (harmonics fall off faster than tanh), 1st-order ADAA, level-normalised drive
  (quiet material and noise floor are never boosted), constant fundamental level at every depth, thermal drift model.
* 5 phase-coherent bands (LR4: 120 / 500 / 2000 / 6500 Hz). Single mode = voiced 5-band engine; Multiband = per-band drive + level.
* Warmth (even/odd balance), Tone tilt, Clarity (2.4-7.5 kHz exciter), Air (wide shelf + exciter), Soft Clip, Mix, in/out gain.
* Spectrum shows input (grey) vs output (colour). Hover a range to freeze + list peaks; click to resume.
