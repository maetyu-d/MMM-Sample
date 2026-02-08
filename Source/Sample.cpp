#include "Sample.h"

bool Sample::loadFromFile(const juce::File& file)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
        return false;
    if (reader->numChannels <= 0 || reader->lengthInSamples <= 0)
        return false;

    sampleRate = reader->sampleRate;
    data.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
    reader->read(&data, 0, (int)reader->lengthInSamples, 0, true, true);
    return true;
}

float Sample::getSampleAt(int channel, double samplePos) const
{
    if (data.getNumSamples() == 0 || data.getNumChannels() == 0)
        return 0.0f;

    channel = juce::jlimit(0, data.getNumChannels() - 1, channel);

    const int numSamples = data.getNumSamples();
    if (samplePos <= 0.0)
        return data.getSample(channel, 0);
    if (samplePos >= (double)(numSamples - 1))
        return data.getSample(channel, numSamples - 1);

    const int index = (int)samplePos;
    const float frac = (float)(samplePos - (double)index);
    const float s0 = data.getSample(channel, index);
    const float s1 = data.getSample(channel, index + 1);
    return s0 + (s1 - s0) * frac;
}
