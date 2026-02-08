#include "WarpCurveEditor.h"
#include <algorithm>

void WarpCurveEditor::setCurve(const WarpCurve& curveIn)
{
    points = curveIn.getPoints();
    if (points.isEmpty())
    {
        points.add({0.0, 0.0});
        points.add({1.0, 1.0});
    }
    selectedIndex = -1;
    repaint();
}

void WarpCurveEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(10.0f);
    g.fillAll(juce::Colour(20, 22, 26));

    g.setColour(juce::Colour(28, 30, 36));
    g.fillRoundedRectangle(bounds, 8.0f);

    // Grid
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    for (int i = 1; i < 4; ++i)
    {
        const float x = bounds.getX() + bounds.getWidth() * (float)i / 4.0f;
        const float y = bounds.getY() + bounds.getHeight() * (float)i / 4.0f;
        g.drawLine(x, bounds.getY(), x, bounds.getBottom());
        g.drawLine(bounds.getX(), y, bounds.getRight(), y);
    }

    // Curve
    g.setColour(juce::Colour(140, 220, 200));
    juce::Path path;
    for (int i = 0; i < points.size(); ++i)
    {
        auto p = pointToView(points.getReference(i));
        if (i == 0)
            path.startNewSubPath(p);
        else
            path.lineTo(p);
    }
    g.strokePath(path, juce::PathStrokeType(2.0f));

    // Points
    for (int i = 0; i < points.size(); ++i)
    {
        auto p = pointToView(points.getReference(i));
        const bool selected = (i == selectedIndex);
        g.setColour(selected ? juce::Colour(240, 220, 120) : juce::Colour(220, 220, 230));
        g.fillEllipse(p.x - 4.0f, p.y - 4.0f, 8.0f, 8.0f);
    }
}

void WarpCurveEditor::mouseDown(const juce::MouseEvent& e)
{
    drawingPath = false;
    const int hit = findPointAt(e.position, 8.0f);
    if (hit >= 0)
    {
        selectedIndex = hit;
        repaint();
        return;
    }

    auto p = viewToPoint(e.position);
    drawingPath = true;
    selectedIndex = -1;
    points.add(p);
    std::sort(points.begin(), points.end(), [](const WarpCurve::Point& a, const WarpCurve::Point& b) { return a.t < b.t; });
    notifyChange();
    repaint();
}

void WarpCurveEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (drawingPath)
    {
        auto p = viewToPoint(e.position);
        if (points.size() < 256)
        {
            const bool shouldAdd = points.isEmpty()
                                   || std::abs(points.getLast().t - p.t) > 0.003
                                   || std::abs(points.getLast().v - p.v) > 0.003;
            if (shouldAdd)
            {
                points.add(p);
                std::sort(points.begin(), points.end(), [](const WarpCurve::Point& a, const WarpCurve::Point& b) { return a.t < b.t; });
                notifyChange();
                repaint();
            }
        }
        return;
    }

    if (selectedIndex < 0 || selectedIndex >= points.size())
        return;

    auto p = viewToPoint(e.position);

    // Lock endpoints in time
    if (selectedIndex == 0)
    {
        p.t = 0.0;
    }
    else if (selectedIndex == points.size() - 1)
    {
        p.t = 1.0;
    }
    else
    {
        const double leftT = points.getReference(selectedIndex - 1).t + 0.001;
        const double rightT = points.getReference(selectedIndex + 1).t - 0.001;
        p.t = juce::jlimit(leftT, rightT, p.t);
        p.v = juce::jlimit(0.0, 1.0, p.v);
    }

    points.set(selectedIndex, p);
    notifyChange();
    repaint();
}

void WarpCurveEditor::mouseUp(const juce::MouseEvent&)
{
    drawingPath = false;
}

void WarpCurveEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
    const int hit = findPointAt(e.position, 8.0f);
    if (hit > 0 && hit < points.size() - 1)
    {
        points.remove(hit);
        selectedIndex = -1;
        notifyChange();
        repaint();
    }
}

int WarpCurveEditor::findPointAt(juce::Point<float> pos, float radius) const
{
    for (int i = 0; i < points.size(); ++i)
    {
        auto p = pointToView(points.getReference(i));
        if (p.getDistanceFrom(pos) <= radius)
            return i;
    }
    return -1;
}

juce::Point<float> WarpCurveEditor::pointToView(const WarpCurve::Point& p) const
{
    auto bounds = getLocalBounds().toFloat().reduced(10.0f);
    const float x = bounds.getX() + (float)p.t * bounds.getWidth();
    const float y = bounds.getBottom() - (float)p.v * bounds.getHeight();
    return {x, y};
}

WarpCurve::Point WarpCurveEditor::viewToPoint(juce::Point<float> p) const
{
    auto bounds = getLocalBounds().toFloat().reduced(10.0f);
    const double t = (p.x - bounds.getX()) / bounds.getWidth();
    const double v = (bounds.getBottom() - p.y) / bounds.getHeight();

    WarpCurve::Point out;
    out.t = juce::jlimit(0.0, 1.0, t);
    out.v = juce::jlimit(0.0, 1.0, v);
    return out;
}

void WarpCurveEditor::notifyChange()
{
    WarpCurve curve;
    curve.setPoints(points);
    if (curveChanged)
        curveChanged(curve);
}
