#pragma once

#include <JuceHeader.h>
#include "Sample.h"

class WaveformComponent : public juce::Component
{
public:
    void setSample(const std::shared_ptr<Sample>& sampleIn);
    void paint(juce::Graphics& g) override;

private:
    std::shared_ptr<Sample> sample;
};
