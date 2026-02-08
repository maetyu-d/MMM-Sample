#include "Track.h"
#include <cmath>
#include <algorithm>
#include <vector>

namespace
{
struct AutomationPointSorter
{
    static int compareElements(TrackAutomationPoint first, TrackAutomationPoint second)
    {
        if (first.samplePosition < second.samplePosition) return -1;
        if (first.samplePosition > second.samplePosition) return 1;
        return 0;
    }
};

juce::Array<TrackAutomationPoint> normalizeAutomation(const juce::Array<TrackAutomationPoint>& in,
                                                      double minValue,
                                                      double maxValue)
{
    juce::Array<TrackAutomationPoint> out = in;
    for (auto& p : out)
    {
        p.samplePosition = juce::jmax<int64>(0, p.samplePosition);
        p.value = juce::jlimit(minValue, maxValue, p.value);
    }
    AutomationPointSorter sorter;
    out.sort(sorter);

    juce::Array<TrackAutomationPoint> dedup;
    for (const auto& p : out)
    {
        if (!dedup.isEmpty() && dedup.getReference(dedup.size() - 1).samplePosition == p.samplePosition)
            dedup.getReference(dedup.size() - 1) = p;
        else
            dedup.add(p);
    }
    return dedup;
}
}

void Track::addClip(const TrackClip& clip)
{
    clips.add(clip);
    ensurePlaybackStateSize();
}

void Track::updateClip(int index, const TrackClip& clip)
{
    if (index < 0 || index >= clips.size())
        return;
    clips.set(index, clip);
    ensurePlaybackStateSize();
}

void Track::clear()
{
    clips.clear();
    playbackStates.clear();
}

void Track::setVolumeAutomation(const juce::Array<TrackAutomationPoint>& points)
{
    volumeAutomation = normalizeAutomation(points, 0.0, 2.0);
}

void Track::setPanAutomation(const juce::Array<TrackAutomationPoint>& points)
{
    panAutomation = normalizeAutomation(points, -1.0, 1.0);
}

void Track::setTimeSignature(int numeratorIn, int denominatorIn)
{
    const int validNumerator = juce::jlimit(1, 32, numeratorIn);
    int validDenominator = denominatorIn;
    if (!(validDenominator == 1 || validDenominator == 2 || validDenominator == 4
          || validDenominator == 8 || validDenominator == 16))
    {
        validDenominator = 4;
    }

    timeSigNumerator = validNumerator;
    timeSigDenominator = validDenominator;
}

int64 Track::getBeatLengthSamples(double sampleRate) const
{
    if (sampleRate <= 0.0)
        return 0;

    const double secondsPerBeatUnit = (60.0 / tempoBpm) * (4.0 / (double)timeSigDenominator);
    return (int64)juce::jmax(1.0, std::round(secondsPerBeatUnit * sampleRate));
}

int64 Track::getBarLengthSamples(double sampleRate) const
{
    const int64 beatSamples = getBeatLengthSamples(sampleRate);
    return beatSamples * (int64)timeSigNumerator;
}

int64 Track::getSnapLengthSamples(double sampleRate) const
{
    const int64 beatSamples = getBeatLengthSamples(sampleRate);
    const int64 barSamples = getBarLengthSamples(sampleRate);

    switch (snapDivision)
    {
        case SnapDivision::bar:
            return juce::jmax<int64>(1, barSamples);
        case SnapDivision::beat:
            return juce::jmax<int64>(1, beatSamples);
        case SnapDivision::eighth:
            return juce::jmax<int64>(1, beatSamples / 2);
        case SnapDivision::sixteenth:
            return juce::jmax<int64>(1, beatSamples / 4);
        case SnapDivision::thirtySecond:
            return juce::jmax<int64>(1, beatSamples / 8);
        case SnapDivision::eighthTriplet:
            return juce::jmax<int64>(1, (beatSamples * 2) / 3);
        case SnapDivision::sixteenthTriplet:
            return juce::jmax<int64>(1, beatSamples / 3);
        default:
            return juce::jmax<int64>(1, beatSamples / 4);
    }
}

int64 Track::snapSampleToGrid(int64 samplePosition, double sampleRate) const
{
    if (!snapEnabled)
        return samplePosition;

    const int64 snapSamples = getSnapLengthSamples(sampleRate);
    if (snapSamples <= 0)
        return samplePosition;

    const double grid = (double)samplePosition / (double)snapSamples;
    const int64 snapped = (int64)std::llround(grid) * snapSamples;
    return juce::jmax<int64>(0, snapped);
}

void Track::setLoopMarker(int markerIndex, int64 samplePosition)
{
    if (markerIndex < 0 || markerIndex > 1)
        return;
    loopMarkerEnabled[markerIndex] = true;
    loopMarkerSamples[markerIndex] = juce::jmax<int64>(0, samplePosition);
}

void Track::clearLoopMarker(int markerIndex)
{
    if (markerIndex < 0 || markerIndex > 1)
        return;
    loopMarkerEnabled[markerIndex] = false;
    loopMarkerSamples[markerIndex] = 0;
}

bool Track::hasLoopMarker(int markerIndex) const
{
    if (markerIndex < 0 || markerIndex > 1)
        return false;
    return loopMarkerEnabled[markerIndex];
}

int64 Track::getLoopMarker(int markerIndex) const
{
    if (markerIndex < 0 || markerIndex > 1)
        return 0;
    return loopMarkerSamples[markerIndex];
}

int Track::getLoopMarkerCount() const
{
    int count = 0;
    if (loopMarkerEnabled[0]) ++count;
    if (loopMarkerEnabled[1]) ++count;
    return count;
}

int64 Track::mapTransportSampleToTrackSample(int64 transportSample) const
{
    transportSample = juce::jmax<int64>(0, transportSample);

    const int count = getLoopMarkerCount();
    if (count == 0)
        return transportSample;

    if (count == 1)
    {
        const int idx = loopMarkerEnabled[0] ? 0 : 1;
        const int64 loopEnd = juce::jmax<int64>(1, loopMarkerSamples[idx]);
        return transportSample % loopEnd;
    }

    const int64 a = loopMarkerSamples[0];
    const int64 b = loopMarkerSamples[1];
    const int64 loopStart = juce::jmin(a, b);
    const int64 loopEnd = juce::jmax(a, b);
    const int64 loopLength = juce::jmax<int64>(1, loopEnd - loopStart);
    return loopStart + (transportSample % loopLength);
}

int64 Track::getClipPlaybackLengthSamples(const TrackClip& clip, double sampleRate) const
{
    if (clip.lengthSamples > 0)
        return clip.lengthSamples;

    const int units = juce::jmax(1, clip.fitLengthUnits);
    const int64 unitSamples = clip.fitToSnapDivision ? getSnapLengthSamples(sampleRate)
                                                     : getBarLengthSamples(sampleRate);
    return juce::jmax<int64>(1, unitSamples * (int64)units);
}

void Track::ensurePlaybackStateSize()
{
    while (playbackStates.size() < clips.size())
        playbackStates.add(ClipPlaybackState{});
    while (playbackStates.size() > clips.size())
        playbackStates.removeLast();
}

void Track::preparePlaybackPlan(int clipIndex)
{
    if (clipIndex < 0 || clipIndex >= clips.size())
        return;

    const auto& clip = clips.getReference(clipIndex);
    if (!clip.slicing.enabled || clip.slicing.slices.isEmpty())
        return;

    auto& state = playbackStates.getReference(clipIndex);
    const int numSlices = clip.slicing.slices.size();

    state.segmentSliceOrder.clearQuick();
    state.segmentActive.clearQuick();
    state.repeatActive.clearQuick();

    juce::Array<int> baseOrder;
    for (int i = 0; i < numSlices; ++i)
        baseOrder.add(i);

    if (clip.slicing.mode == SlicePlaybackMode::rotate)
    {
        const int offset = ((state.rotationOffset % numSlices) + numSlices) % numSlices;
        for (int i = 0; i < numSlices; ++i)
            state.segmentSliceOrder.add(baseOrder[(i + offset) % numSlices]);
        state.rotationOffset = (state.rotationOffset + 1) % numSlices;
    }
    else if (clip.slicing.mode == SlicePlaybackMode::random)
    {
        while (!baseOrder.isEmpty())
        {
            const int idx = random.nextInt(baseOrder.size());
            state.segmentSliceOrder.add(baseOrder[idx]);
            baseOrder.remove(idx);
        }
    }
    else
    {
        state.segmentSliceOrder = baseOrder;
    }

    for (int seg = 0; seg < numSlices; ++seg)
    {
        const int sliceIndex = state.segmentSliceOrder[seg];
        const auto& slice = clip.slicing.slices.getReference(sliceIndex);

        const bool active = random.nextDouble() <= juce::jlimit(0.0, 1.0, slice.playProbability);
        state.segmentActive.add(active);

        juce::Array<bool> repeatFlags;
        const int repeats = juce::jmax(1, slice.ratchetRepeats);
        for (int r = 0; r < repeats; ++r)
        {
            if (r == 0)
                repeatFlags.add(true);
            else
                repeatFlags.add(random.nextDouble() <= juce::jlimit(0.0, 1.0, slice.repeatProbability));
        }
        state.repeatActive.add(repeatFlags);
    }
}

float Track::renderSlicedSample(const TrackClip& clip, const ClipPlaybackState& state, double localNorm, int channel) const
{
    const auto& slices = clip.slicing.slices;
    const int numSlices = slices.size();
    if (numSlices == 0)
        return 0.0f;

    const double scaled = juce::jlimit(0.0, 0.999999, localNorm) * (double)numSlices;
    const int segmentIndex = juce::jlimit(0, numSlices - 1, (int)scaled);
    const double segmentLocal = scaled - (double)segmentIndex;

    if (segmentIndex >= state.segmentActive.size() || !state.segmentActive[segmentIndex])
        return 0.0f;

    if (segmentIndex >= state.segmentSliceOrder.size())
        return 0.0f;

    const int sliceIndex = state.segmentSliceOrder[segmentIndex];
    if (sliceIndex < 0 || sliceIndex >= slices.size())
        return 0.0f;

    const auto& slice = slices.getReference(sliceIndex);
    const int repeats = juce::jmax(1, slice.ratchetRepeats);
    const int repeatIndex = juce::jlimit(0, repeats - 1, (int)(segmentLocal * (double)repeats));

    if (segmentIndex < state.repeatActive.size())
    {
        const auto& repeatFlags = state.repeatActive.getReference(segmentIndex);
        if (repeatIndex < repeatFlags.size() && !repeatFlags[repeatIndex])
            return 0.0f;
    }

    const double repeatLocal = juce::jlimit(0.0, 1.0, segmentLocal * (double)repeats - (double)repeatIndex);
    const double clipSourceStart = juce::jlimit(0.0, 1.0, (double)clip.sourceStartNorm);
    const double clipSourceEnd = juce::jlimit(clipSourceStart, 1.0, (double)clip.sourceEndNorm);
    const double clipSourceRange = juce::jmax(0.000001, clipSourceEnd - clipSourceStart);
    const double startNorm = juce::jlimit(0.0, 1.0, slice.startNorm);
    const double endNorm = juce::jlimit(startNorm, 1.0, slice.endNorm);
    const double sliceNorm = juce::jmap(repeatLocal, startNorm, endNorm);
    const double clipNorm = clipSourceStart + clipSourceRange * sliceNorm;
    const double warpedNorm = clip.warpCurve.evaluate(clipNorm);
    const double samplePos = warpedNorm * (double)(clip.sample->getNumSamples() - 1);
    return clip.sample->getSampleAt(channel, samplePos);
}

double Track::evaluateAutomation(const juce::Array<TrackAutomationPoint>& automation,
                                 int64 samplePosition,
                                 double defaultValue,
                                 double minValue,
                                 double maxValue) const
{
    if (automation.isEmpty())
        return juce::jlimit(minValue, maxValue, defaultValue);

    if (samplePosition <= automation.getReference(0).samplePosition)
        return juce::jlimit(minValue, maxValue, automation.getReference(0).value);

    for (int i = 1; i < automation.size(); ++i)
    {
        const auto& a = automation.getReference(i - 1);
        const auto& b = automation.getReference(i);
        if (samplePosition <= b.samplePosition)
        {
            const int64 span = juce::jmax<int64>(1, b.samplePosition - a.samplePosition);
            const double t = (double)(samplePosition - a.samplePosition) / (double)span;
            return juce::jlimit(minValue, maxValue, juce::jmap(t, a.value, b.value));
        }
    }

    return juce::jlimit(minValue, maxValue, automation.getReference(automation.size() - 1).value);
}

void Track::render(juce::AudioBuffer<float>& buffer, int64 bufferStartSample, double sampleRate)
{
    if (muted)
        return;

    ensurePlaybackStateSize();

    const int numOutChannels = buffer.getNumChannels();
    const int numOutSamples = buffer.getNumSamples();
    if (numOutSamples <= 0 || clips.isEmpty())
        return;

    std::vector<char> wasInside((size_t)clips.size(), 0);
    const int64 prevTrackSample = mapTransportSampleToTrackSample(juce::jmax<int64>(0, bufferStartSample - 1));
    for (int ci = 0; ci < clips.size(); ++ci)
    {
        const auto& clip = clips.getReference(ci);
        const int64 clipLenSamples = getClipPlaybackLengthSamples(clip, sampleRate);
        const int64 clipEnd = clip.startSample + clipLenSamples;
        wasInside[(size_t)ci] = (prevTrackSample >= clip.startSample && prevTrackSample < clipEnd) ? 1 : 0;
    }

    for (int outIndex = 0; outIndex < numOutSamples; ++outIndex)
    {
        const int64 transportSample = bufferStartSample + outIndex;
        const int64 trackSample = mapTransportSampleToTrackSample(transportSample);
        const double volNow = evaluateAutomation(volumeAutomation, trackSample, volume, 0.0, 2.0);
        const double panNow = evaluateAutomation(panAutomation, trackSample, pan, -1.0, 1.0);
        const double panAngle = (panNow + 1.0) * juce::MathConstants<double>::pi * 0.25;
        const float leftTrackGain = (float)(std::cos(panAngle) * volNow);
        const float rightTrackGain = (float)(std::sin(panAngle) * volNow);

        for (int clipIndex = 0; clipIndex < clips.size(); ++clipIndex)
        {
            const auto& clip = clips.getReference(clipIndex);
            if (clip.muted || clip.sample == nullptr || clip.lengthSamples <= 0)
            {
                wasInside[(size_t)clipIndex] = 0;
                continue;
            }

            const int64 clipLenSamples = getClipPlaybackLengthSamples(clip, sampleRate);

            const int64 clipEnd = clip.startSample + clipLenSamples;
            const bool inside = (trackSample >= clip.startSample && trackSample < clipEnd);
            if (inside && !wasInside[(size_t)clipIndex] && clip.slicing.enabled && !clip.slicing.slices.isEmpty())
                preparePlaybackPlan(clipIndex);

            if (!inside)
            {
                wasInside[(size_t)clipIndex] = 0;
                continue;
            }

            const int sampleCount = clip.sample->getNumSamples();
            const int sampleChannels = clip.sample->getNumChannels();
            if (sampleCount <= 0 || sampleChannels <= 0)
            {
                wasInside[(size_t)clipIndex] = 1;
                continue;
            }

            const double local = (double)(trackSample - clip.startSample) / (double)clipLenSamples;
            const auto& playbackState = playbackStates.getReference(clipIndex);

            for (int ch = 0; ch < numOutChannels; ++ch)
            {
                const int sampleChannel = juce::jmin(ch, sampleChannels - 1);
                float s = 0.0f;
                if (clip.slicing.enabled && !clip.slicing.slices.isEmpty())
                {
                    s = renderSlicedSample(clip, playbackState, local, sampleChannel);
                }
                else
                {
                    const double clipSourceStart = juce::jlimit(0.0, 1.0, (double)clip.sourceStartNorm);
                    const double clipSourceEnd = juce::jlimit(clipSourceStart, 1.0, (double)clip.sourceEndNorm);
                    const double clipNorm = juce::jmap(local, clipSourceStart, clipSourceEnd);
                    const double inputNorm = clip.warpCurve.evaluate(clipNorm);
                    const double inputPos = inputNorm * (double)(sampleCount - 1);
                    s = clip.sample->getSampleAt(sampleChannel, inputPos);
                }

                float trackGain = 1.0f;
                if (numOutChannels >= 2)
                    trackGain = (ch == 0 ? leftTrackGain : (ch == 1 ? rightTrackGain : (float)volNow));
                else
                    trackGain = (float)volNow;

                const float fadeIn = juce::jlimit(0.0f, 0.98f, clip.fadeInNorm);
                const float fadeOut = juce::jlimit(0.0f, 0.98f - fadeIn, clip.fadeOutNorm);
                float fadeGain = 1.0f;
                if (fadeIn > 0.0f)
                    fadeGain = juce::jmin(fadeGain, (float)juce::jlimit(0.0, 1.0, local / (double)fadeIn));
                if (fadeOut > 0.0f)
                    fadeGain = juce::jmin(fadeGain, (float)juce::jlimit(0.0, 1.0, (1.0 - local) / (double)fadeOut));

                buffer.addSample(ch, outIndex, s * clip.gain * fadeGain * trackGain);
            }

            wasInside[(size_t)clipIndex] = 1;
        }
    }
}
