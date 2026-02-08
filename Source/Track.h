#pragma once

#include <JuceHeader.h>
#include "Sample.h"
#include "WarpCurve.h"

enum class SlicePlaybackMode
{
    sequential = 0,
    rotate = 1,
    random = 2
};

struct BeatSlice
{
    double startNorm = 0.0;
    double endNorm = 1.0;
    double playProbability = 1.0;
    int ratchetRepeats = 1;
    double repeatProbability = 1.0;
};

struct BeatSlicingSettings
{
    bool enabled = false;
    SlicePlaybackMode mode = SlicePlaybackMode::sequential;
    juce::Array<BeatSlice> slices;
};

struct TrackAutomationPoint
{
    int64 samplePosition = 0;
    double value = 0.0;
};

struct TrackClip
{
    juce::String name;
    juce::String sourceFilePath;
    std::shared_ptr<Sample> sample;
    WarpCurve warpCurve;
    BeatSlicingSettings slicing;
    int64 startSample = 0;      // timeline sample offset
    int64 lengthSamples = 0;    // timeline duration
    float sourceStartNorm = 0.0f; // source range start [0..1]
    float sourceEndNorm = 1.0f;   // source range end [0..1]
    int fitLengthUnits = 1;     // bars or snap units
    bool fitToSnapDivision = false;
    float gain = 1.0f;
    float fadeInNorm = 0.0f;    // portion of clip [0..1]
    float fadeOutNorm = 0.0f;   // portion of clip [0..1]
    bool muted = false;
};

class Track
{
public:
    enum class SnapDivision
    {
        bar = 0,
        beat = 1,
        eighth = 2,
        sixteenth = 3,
        thirtySecond = 4,
        eighthTriplet = 5,
        sixteenthTriplet = 6
    };

    Track(const juce::String& nameIn) : name(nameIn) {}

    void addClip(const TrackClip& clip);
    void updateClip(int index, const TrackClip& clip);
    void clear();

    const juce::String& getName() const { return name; }
    void setName(const juce::String& newName) { name = newName; }
    const juce::Array<TrackClip>& getClips() const { return clips; }

    void setTempoBpm(double bpmIn) { tempoBpm = juce::jlimit(20.0, 320.0, bpmIn); }
    double getTempoBpm() const { return tempoBpm; }

    void setTimeSignature(int numeratorIn, int denominatorIn);
    int getTimeSigNumerator() const { return timeSigNumerator; }
    int getTimeSigDenominator() const { return timeSigDenominator; }

    void setSnapEnabled(bool shouldSnap) { snapEnabled = shouldSnap; }
    bool isSnapEnabled() const { return snapEnabled; }
    void setSnapDivision(SnapDivision division) { snapDivision = division; }
    SnapDivision getSnapDivision() const { return snapDivision; }

    void setZoomX(double zoomIn) { zoomX = juce::jlimit(0.25, 8.0, zoomIn); }
    double getZoomX() const { return zoomX; }
    void setZoomY(double zoomIn) { zoomY = juce::jlimit(0.5, 3.0, zoomIn); }
    double getZoomY() const { return zoomY; }
    void setVolume(double volumeIn) { volume = juce::jlimit(0.0, 2.0, volumeIn); }
    double getVolume() const { return volume; }
    void setPan(double panIn) { pan = juce::jlimit(-1.0, 1.0, panIn); }
    double getPan() const { return pan; }
    void setVolumeAutomation(const juce::Array<TrackAutomationPoint>& points);
    void setPanAutomation(const juce::Array<TrackAutomationPoint>& points);
    const juce::Array<TrackAutomationPoint>& getVolumeAutomation() const { return volumeAutomation; }
    const juce::Array<TrackAutomationPoint>& getPanAutomation() const { return panAutomation; }

    int64 getBeatLengthSamples(double sampleRate) const;
    int64 getBarLengthSamples(double sampleRate) const;
    int64 getSnapLengthSamples(double sampleRate) const;
    int64 snapSampleToGrid(int64 samplePosition, double sampleRate) const;
    void setLoopMarker(int markerIndex, int64 samplePosition);
    void clearLoopMarker(int markerIndex);
    bool hasLoopMarker(int markerIndex) const;
    int64 getLoopMarker(int markerIndex) const;
    int getLoopMarkerCount() const;
    int64 mapTransportSampleToTrackSample(int64 transportSample) const;
    int64 getClipPlaybackLengthSamples(const TrackClip& clip, double sampleRate) const;

    void render(juce::AudioBuffer<float>& buffer, int64 bufferStartSample, double sampleRate);

    bool muted = false;

private:
    struct ClipPlaybackState
    {
        int rotationOffset = 0;
        juce::Array<int> segmentSliceOrder;
        juce::Array<bool> segmentActive;
        juce::Array<juce::Array<bool>> repeatActive;
    };

    void ensurePlaybackStateSize();
    void preparePlaybackPlan(int clipIndex);
    float renderSlicedSample(const TrackClip& clip, const ClipPlaybackState& state, double localNorm, int channel) const;
    double evaluateAutomation(const juce::Array<TrackAutomationPoint>& automation,
                              int64 samplePosition,
                              double defaultValue,
                              double minValue,
                              double maxValue) const;

    juce::String name;
    juce::Array<TrackClip> clips;
    juce::Array<ClipPlaybackState> playbackStates;
    mutable juce::Random random;
    double tempoBpm = 120.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
    bool snapEnabled = true;
    SnapDivision snapDivision = SnapDivision::sixteenth;
    double zoomX = 1.0;
    double zoomY = 1.0;
    double volume = 1.0;
    double pan = 0.0;
    juce::Array<TrackAutomationPoint> volumeAutomation;
    juce::Array<TrackAutomationPoint> panAutomation;
    bool loopMarkerEnabled[2] { false, false };
    int64 loopMarkerSamples[2] { 0, 0 };
};
