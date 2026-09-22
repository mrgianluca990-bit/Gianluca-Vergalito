# Prima Take Finish v0.2

JUCE audio plugin prototype for macOS.

## Formats
- VST3
- AU
- Standalone

## DSP chain
Drive -> Body -> Presence -> Control -> Mix -> Output

## Automatic macOS build
The repository includes `.github/workflows/build-macos.yml`.
On every push to `main`, or manual workflow dispatch, GitHub Actions builds a universal macOS binary (`arm64 + x86_64`) using JUCE 9.0.2.

Artifacts produced:
- `PrimaTakeFinish-VST3-macOS-universal.zip`
- `PrimaTakeFinish-AU-macOS-universal.zip`
- `PrimaTakeFinish-Standalone-macOS-universal.zip`

These CI artifacts are ad-hoc signed for local testing, not notarized for public distribution.

## Manual macOS build
```bash
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build --config Release
```

JUCE is fetched automatically by CMake; no manual JUCE installation is required.
