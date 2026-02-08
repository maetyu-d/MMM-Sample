#pragma once

#include <JuceHeader.h>

class Sample
{
public:
    bool loadFromFile(const juce::File& file);

    int getNumChannels() const { return data.getNumChannels(); }
    int getNumSamples() const { return data.getNumSamples(); }
    double getSampleRate() const { return sampleRate; }

    float getSampleAt(int channel, double samplePos) const;

private:
    juce::AudioBuffer<float> data;
    double sampleRate = 44100.0;
};
