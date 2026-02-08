#pragma once

#include <JuceHeader.h>
#include "WarpCurve.h"

class WarpCurveEditor : public juce::Component
{
public:
    using CurveChanged = std::function<void(const WarpCurve&)>;

    void setCurve(const WarpCurve& curveIn);
    void onCurveChanged(CurveChanged cb) { curveChanged = std::move(cb); }

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    int findPointAt(juce::Point<float> pos, float radius) const;
    juce::Point<float> pointToView(const WarpCurve::Point& p) const;
    WarpCurve::Point viewToPoint(juce::Point<float> p) const;

    void notifyChange();

    juce::Array<WarpCurve::Point> points;
    int selectedIndex = -1;
    bool drawingPath = false;
    CurveChanged curveChanged;
};
