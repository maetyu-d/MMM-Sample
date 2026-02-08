# MMM Sample

Fundamentally, MMM Sample is a JUCE-based application for building compositions from disparate WAV samples with automatic time-warped resampling so that they always fits a target structure/length.

## Build (macOS)

1. Download JUCE and place it next to this project folder as `./JUCE`, or set `-DJUCE_DIR=/path/to/JUCE`.
2. Configure and build:

```bash
cmake -B build -S .
cmake --build build
```

## What works so far

- Load a WAV into a track
- Render timeline clips with variable resampling curves (Linear, Slow->Fast, Fast->Slow)
- DAW-like arrangement view with tracks and clips

## Next steps

- Add clip-level warp editor (custom points)
- Add multiple tracks, drag/drop, and timeline snapping
- Add transport/tempo grid and playback loop
