# MMM Sample

MMM Sample is a JUCE-based application for building audio compositions from disparate WAV samples, with automatic time-warped resampling to ensure that samples always fit a target structure or duration, even if tempo and/or time signature are radically and/or repeatedly altered. Additionally, there is no requirement for resampling to be applied linearly across a sample, and resampling rate can be warped to follow any relative path set out by the user. For example, the start of a sample can be made to play more quickly than the end, or, with a sawtooth-like path applied, more quickly, then more slowly, then more quickly, then more slowly, etc., while automatically making compensations for any change to guarantee overall sample duration. Note that there is no pitch compensation, and so the application is primarily intended for assemblages of disparate rhythmic rather than melodic material, but play around and use it however you see fit. You can, for instance, get some My Bloody Valentine glide guitar-type effects on pitched material (which was actually the main reason I made this)!

![]([https://github.com/maetyu-d/MMM-Sample/blob/main/Screenshot%202026-02-08%20at%2020.30.03.png)


## Build (macOS)

1. Download JUCE and place it next to this project folder as `./JUCE`, or set `-DJUCE_DIR=/path/to/JUCE`.
2. Configure and build:

```bash
cmake -B build -S .
cmake --build build
```

