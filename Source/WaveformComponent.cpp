#include "WaveformComponent.h"

void WaveformComponent::setSample(const std::shared_ptr<Sample>& sampleIn)
{
    sample = sampleIn;
    repaint();
}

void WaveformComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(18, 20, 24));
    g.setColour(juce::Colours::white.withAlpha(0.2f));

    if (sample == nullptr || sample->getNumSamples() == 0)
        return;

    const int numSamples = sample->getNumSamples();
    const int width = getWidth();
    const int height = getHeight();
    const int mid = height / 2;

    for (int x = 0; x < width; ++x)
    {
        const double t = (double)x / (double)(width - 1);
        const double samplePos = t * (double)(numSamples - 1);
        const float s = sample->getSampleAt(0, samplePos);
        const int y = (int)(s * (float)(mid - 4));
        g.drawLine((float)x, (float)(mid - y), (float)x, (float)(mid + y));
    }
}
