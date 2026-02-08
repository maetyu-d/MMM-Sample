#include "AudioEngine.h"
#include <vector>

void AudioEngine::prepare(double sampleRateIn, int)
{
    sampleRate = sampleRateIn;
    playheadSample.store(0);
}

void AudioEngine::release()
{
}

void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();

    if (info.buffer == nullptr || info.numSamples <= 0)
        return;

    if (!playing.load())
        return;

    const int numChannels = info.buffer->getNumChannels();
    if (numChannels <= 0)
        return;

    std::vector<float*> channelPointers;
    channelPointers.resize((size_t)numChannels);
    for (int ch = 0; ch < numChannels; ++ch)
        channelPointers[(size_t)ch] = info.buffer->getWritePointer(ch, info.startSample);

    juce::AudioBuffer<float> activeBuffer(channelPointers.data(), numChannels, info.numSamples);

    juce::ScopedLock lock(trackLock);
    int64 ph = playheadSample.load();

    const int64 bufferStart = ph;
    for (auto& track : tracks)
        track.render(activeBuffer, bufferStart, sampleRate);

    ph += info.numSamples;
    playheadSample.store(ph);
}

void AudioEngine::setPlaying(bool shouldPlay)
{
    playing.store(shouldPlay);
}

void AudioEngine::addTrack(const juce::String& name)
{
    juce::ScopedLock lock(trackLock);
    tracks.add(Track(name));
}

void AudioEngine::clearTracks()
{
    juce::ScopedLock lock(trackLock);
    tracks.clear();
}
