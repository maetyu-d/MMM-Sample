#pragma once

#include <JuceHeader.h>

class WarpCurve
{
public:
    struct Point
    {
        double t = 0.0;  // output time [0..1]
        double v = 0.0;  // input time  [0..1]
    };

    WarpCurve();

    void setPoints(const juce::Array<Point>& newPoints);
    const juce::Array<Point>& getPoints() const { return points; }

    void setRelativeMode(bool enabled);
    bool isRelativeMode() const { return relativeMode; }
    void setSmoothRateChanges(bool enabled);
    bool getSmoothRateChanges() const { return smoothRateChanges; }

    double evaluate(double t) const;

    static WarpCurve linear();
    static WarpCurve slowToFast();
    static WarpCurve fastToSlow();

private:
    void markDirty();
    void rebuildLookupIfNeeded() const;
    static double evaluatePoints(const juce::Array<Point>& pointsIn, double t, bool smoothSegments);
    static double pointValueToRate(double v);

    juce::Array<Point> points;
    bool relativeMode = true;
    bool smoothRateChanges = false;
    mutable bool lookupDirty = true;
    mutable juce::Array<double> lookup;
};
