#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "Track.h"

class AudioEngine
{
public:
    void prepare(double sampleRateIn, int samplesPerBlockExpected);
    void release();

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info);

    void setPlaying(bool shouldPlay);
    bool isPlaying() const { return playing.load(); }

    void setPlayheadSample(int64 newPos) { playheadSample.store(newPos); }
    int64 getPlayheadSample() const { return playheadSample.load(); }

    void addTrack(const juce::String& name);
    void clearTracks();

    juce::Array<Track>& getTracks() { return tracks; }
    const juce::Array<Track>& getTracks() const { return tracks; }
    juce::CriticalSection& getTrackLock() { return trackLock; }

private:
    double sampleRate = 44100.0;
    std::atomic<int64> playheadSample {0};
    std::atomic<bool> playing {false};

    juce::CriticalSection trackLock;
    juce::Array<Track> tracks;
};
