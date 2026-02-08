#include "WarpCurve.h"
#include <algorithm>
#include <cmath>

WarpCurve::WarpCurve()
{
    points.add({0.0, 0.5});
    points.add({1.0, 0.5});
}

void WarpCurve::setPoints(const juce::Array<Point>& newPoints)
{
    points.clear();
    if (newPoints.isEmpty())
    {
        points.add({0.0, 0.5});
        points.add({1.0, 0.5});
        markDirty();
        return;
    }

    for (const auto& p : newPoints)
        points.add(p);

    std::sort(points.begin(), points.end(), [](const Point& a, const Point& b) { return a.t < b.t; });

    if (points.getFirst().t > 0.0)
        points.insert(0, {0.0, points.getFirst().v});
    if (points.getLast().t < 1.0)
        points.add({1.0, points.getLast().v});

    for (auto& p : points)
    {
        p.t = juce::jlimit(0.0, 1.0, p.t);
        p.v = juce::jlimit(0.0, 1.0, p.v);
    }

    points.getReference(0).t = 0.0;
    points.getReference(points.size() - 1).t = 1.0;
    markDirty();
}

void WarpCurve::setRelativeMode(bool enabled)
{
    if (relativeMode == enabled)
        return;
    relativeMode = enabled;
    markDirty();
}

void WarpCurve::setSmoothRateChanges(bool enabled)
{
    if (smoothRateChanges == enabled)
        return;
    smoothRateChanges = enabled;
    markDirty();
}

double WarpCurve::evaluatePoints(const juce::Array<Point>& pointsIn, double t, bool smoothSegments)
{
    if (pointsIn.isEmpty())
        return t;

    const double tt = juce::jlimit(0.0, 1.0, t);
    for (int i = 0; i < pointsIn.size() - 1; ++i)
    {
        const auto& a = pointsIn.getReference(i);
        const auto& b = pointsIn.getReference(i + 1);
        if (tt >= a.t && tt <= b.t)
        {
            const double span = b.t - a.t;
            if (span <= 0.0)
                return a.v;
            double alpha = (tt - a.t) / span;
            if (smoothSegments)
                alpha = alpha * alpha * (3.0 - 2.0 * alpha);
            return juce::jmap(alpha, a.v, b.v);
        }
    }

    return pointsIn.getLast().v;
}

double WarpCurve::pointValueToRate(double v)
{
    const double vv = juce::jlimit(0.0, 1.0, v);
    return 0.5 + vv;
}

void WarpCurve::markDirty()
{
    lookupDirty = true;
}

void WarpCurve::rebuildLookupIfNeeded() const
{
    if (!lookupDirty)
        return;

    lookup.clear();
    constexpr int tableSize = 1025;
    lookup.resize(tableSize);

    if (points.isEmpty())
    {
        for (int i = 0; i < tableSize; ++i)
            lookup.set(i, (double)i / (double)(tableSize - 1));
        lookupDirty = false;
        return;
    }

    if (!relativeMode)
    {
        for (int i = 0; i < tableSize; ++i)
        {
            const double t = (double)i / (double)(tableSize - 1);
            lookup.set(i, juce::jlimit(0.0, 1.0, evaluatePoints(points, t, false)));
        }
        lookup.set(0, 0.0);
        lookup.set(tableSize - 1, 1.0);
        lookupDirty = false;
        return;
    }

    juce::Array<double> cumulative;
    cumulative.resize(tableSize);
    cumulative.set(0, 0.0);

    double lastRate = pointValueToRate(evaluatePoints(points, 0.0, smoothRateChanges));
    double sum = 0.0;
    for (int i = 1; i < tableSize; ++i)
    {
        const double t = (double)i / (double)(tableSize - 1);
        const double rate = pointValueToRate(evaluatePoints(points, t, smoothRateChanges));
        sum += 0.5 * (lastRate + rate);
        cumulative.set(i, sum);
        lastRate = rate;
    }

    const double total = juce::jmax(0.000001, cumulative.getLast());
    for (int i = 0; i < tableSize; ++i)
        lookup.set(i, juce::jlimit(0.0, 1.0, cumulative.getReference(i) / total));

    lookup.set(0, 0.0);
    lookup.set(tableSize - 1, 1.0);
    lookupDirty = false;
}

double WarpCurve::evaluate(double t) const
{
    rebuildLookupIfNeeded();
    if (lookup.isEmpty())
        return juce::jlimit(0.0, 1.0, t);

    const double tt = juce::jlimit(0.0, 1.0, t);
    const double pos = tt * (double)(lookup.size() - 1);
    const int idx = juce::jlimit(0, lookup.size() - 2, (int)std::floor(pos));
    const double frac = pos - (double)idx;
    const double a = lookup.getReference(idx);
    const double b = lookup.getReference(idx + 1);
    return juce::jmap(frac, a, b);
}

WarpCurve WarpCurve::linear()
{
    WarpCurve c;
    juce::Array<Point> pts;
    pts.add({0.0, 0.5});
    pts.add({1.0, 0.5});
    c.setPoints(pts);
    return c;
}

WarpCurve WarpCurve::slowToFast()
{
    WarpCurve c;
    juce::Array<Point> pts;
    pts.add({0.0, 0.25});
    pts.add({0.35, 0.40});
    pts.add({0.7, 0.62});
    pts.add({1.0, 0.80});
    c.setPoints(pts);
    return c;
}

WarpCurve WarpCurve::fastToSlow()
{
    WarpCurve c;
    juce::Array<Point> pts;
    pts.add({0.0, 0.80});
    pts.add({0.35, 0.62});
    pts.add({0.7, 0.40});
    pts.add({1.0, 0.25});
    c.setPoints(pts);
    return c;
}
