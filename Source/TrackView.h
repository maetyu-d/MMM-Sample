#pragma once

#include <JuceHeader.h>
#include "Track.h"

class TrackView : public juce::Component
{
public:
    void setTrack(const Track* trackIn) { track = trackIn; repaint(); }
    void paint(juce::Graphics& g) override;

private:
    const Track* track = nullptr;
};
