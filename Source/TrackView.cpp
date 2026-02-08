#include "TrackView.h"

void TrackView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(24, 26, 30));
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    if (track != nullptr)
        g.drawText(track->getName(), 10, 0, getWidth() - 20, getHeight(), juce::Justification::centredLeft);
}
