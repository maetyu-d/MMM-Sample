#include "ArrangementView.h"
#include <cmath>
#include <limits>

ArrangementView::ArrangementView()
{
    setWantsKeyboardFocus(true);
    addAndMakeVisible(inlineEditor);
    inlineEditor.setVisible(false);
    inlineEditor.setMultiLine(false);
    inlineEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(24, 28, 36));
    inlineEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(210, 185, 120));
    inlineEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(240, 220, 120));
    inlineEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    inlineEditor.onReturnKey = [this]() { finishInlineEdit(); };
    inlineEditor.onFocusLost = [this]() { finishInlineEdit(); };
    inlineEditor.onEscapeKey = [this]()
    {
        suppressInlineCommit = true;
        inlineEditor.setVisible(false);
        editingField = EditField::none;
        editingTrack = -1;
        suppressInlineCommit = false;
    };
}

void ArrangementView::setTracks(const juce::Array<Track>* tracksIn, double sampleRateIn)
{
    tracks = tracksIn;
    sampleRate = sampleRateIn;
    ensureAutomationExpandedState();
    ensureZoomSliders();
    layoutZoomSliders();
    repaint();
}

void ArrangementView::setZoomSecondsPerPixel(double secondsPerPixelIn)
{
    secondsPerPixel = juce::jlimit(0.0005, 0.05, secondsPerPixelIn);
    repaint();
}

void ArrangementView::setSelectedClip(int trackIndexIn, int clipIndexIn)
{
    selectedTrack = trackIndexIn;
    selectedClip = clipIndexIn;
    if (clipIndexIn >= 0 && trackIndexIn >= 0)
        setSingleClipSelection(trackIndexIn, clipIndexIn);
    else if (trackIndexIn >= 0)
        setSingleTrackSelection(trackIndexIn);
    else
    {
        selectionType = SelectionType::none;
        selectedTracks.clear();
        selectedClips.clear();
        selectedMarkers.clear();
    }
    repaint();
}

void ArrangementView::setPlayheadSample(int64 playheadIn)
{
    playheadSample = juce::jmax<int64>(0, playheadIn);
    repaint();
}

void ArrangementView::refreshTrackControls()
{
    ensureAutomationExpandedState();
    ensureZoomSliders();
    layoutZoomSliders();
    repaint();
}

int ArrangementView::getRequiredContentHeight() const
{
    int total = headerHeight;

    if (tracks != nullptr)
    {
        for (int i = 0; i < tracks->size(); ++i)
            total += getTrackHeight(i);
    }
    else
    {
        total += trackHeight;
    }

    return juce::jmax(total, headerHeight + 1);
}

int ArrangementView::getTrackHeight(int trackIndex) const
{
    if (tracks == nullptr || trackIndex < 0 || trackIndex >= tracks->size())
        return trackHeight;

    const auto& t = tracks->getReference(trackIndex);
    const int mainLaneHeight = juce::jmax(108, (int)std::llround((double)trackHeight * t.getZoomY()));
    int h = mainLaneHeight;
    if (trackIndex < automationExpanded.size() && automationExpanded.getUnchecked(trackIndex))
        h = mainLaneHeight * 2;
    return h;
}

int ArrangementView::getTimelineStartX() const
{
    return trackControlsWidth + trackHeaderWidth + timelineStartGap;
}

int ArrangementView::getTrackTop(int trackIndex) const
{
    int y = headerHeight;
    if (tracks == nullptr)
        return y;

    for (int i = 0; i < trackIndex && i < tracks->size(); ++i)
        y += getTrackHeight(i);
    return y;
}

int ArrangementView::yToTrackIndex(float y) const
{
    if (tracks == nullptr || y < (float)headerHeight)
        return -1;

    int cursor = headerHeight;
    for (int i = 0; i < tracks->size(); ++i)
    {
        const int h = getTrackHeight(i);
        if ((int)y >= cursor && (int)y < cursor + h)
            return i;
        cursor += h;
    }

    return -1;
}

int64 ArrangementView::xToSampleForTrack(int trackIndex, float x) const
{
    if (tracks == nullptr || trackIndex < 0 || trackIndex >= tracks->size())
        return 0;

    const auto& t = tracks->getReference(trackIndex);
    const int timelineStartX = getTimelineStartX();
    const double localSecondsPerPixel = secondsPerPixel / t.getZoomX();
    const double timelineRight = juce::jmax((double)timelineStartX, (double)getWidth());
    const double clampedX = juce::jlimit((double)timelineStartX, timelineRight, (double)x);
    const double timelineX = juce::jmax(0.0, clampedX - (double)timelineStartX);
    const double seconds = timelineX * localSecondsPerPixel;
    return (int64)std::llround(seconds * sampleRate);
}

int ArrangementView::sampleToXForTrack(int trackIndex, int64 samplePosition) const
{
    if (tracks == nullptr || trackIndex < 0 || trackIndex >= tracks->size())
        return trackHeaderWidth + trackControlsWidth;

    const auto& t = tracks->getReference(trackIndex);
    const int timelineStartX = getTimelineStartX();
    const double localSecondsPerPixel = secondsPerPixel / t.getZoomX();
    const double seconds = (double)samplePosition / sampleRate;
    const int x = timelineStartX + (int)std::llround(seconds / localSecondsPerPixel);
    return juce::jmin(x, getWidth());
}

juce::Rectangle<int> ArrangementView::getTrackHeaderBounds(int trackIndex) const
{
    const int y = getTrackTop(trackIndex);
    return {trackControlsWidth, y, trackHeaderWidth, getTrackHeight(trackIndex) - 1};
}

juce::Rectangle<int> ArrangementView::getNameFieldBounds(int trackIndex) const
{
    auto r = getTrackHeaderBounds(trackIndex).reduced(8, 8);
    return r.removeFromTop(24);
}

juce::Rectangle<int> ArrangementView::getTempoFieldBounds(int trackIndex) const
{
    auto r = getTrackHeaderBounds(trackIndex).reduced(8, 8);
    r.removeFromTop(30);
    return r.removeFromLeft(84);
}

juce::Rectangle<int> ArrangementView::getTimeSigNumFieldBounds(int trackIndex) const
{
    auto r = getTrackHeaderBounds(trackIndex).reduced(8, 8);
    r.removeFromTop(30);
    r.removeFromLeft(94);
    return r.removeFromLeft(34);
}

juce::Rectangle<int> ArrangementView::getTimeSigDenFieldBounds(int trackIndex) const
{
    auto r = getTrackHeaderBounds(trackIndex).reduced(8, 8);
    r.removeFromTop(30);
    r.removeFromLeft(132);
    return r.removeFromLeft(34);
}

juce::Rectangle<int> ArrangementView::getAutomationToggleBounds(int trackIndex) const
{
    auto r = getTrackHeaderBounds(trackIndex).reduced(8, 8);
    return { r.getRight() - 24, r.getY(), 24, 24 };
}

juce::Rectangle<int> ArrangementView::getClipLaneBounds(int trackIndex) const
{
    const int y = getTrackTop(trackIndex);
    const int h = getTrackHeight(trackIndex);
    const bool expanded = (trackIndex < automationExpanded.size() && automationExpanded.getUnchecked(trackIndex));
    const int clipTop = y + 12;
    const int clipBottom = expanded ? (y + (h / 2) - 6) : (y + h - 12);
    return { 0, clipTop, getWidth(), juce::jmax(18, clipBottom - clipTop) };
}

juce::Rectangle<int> ArrangementView::getAutomationLaneBounds(int trackIndex, bool volumeLane) const
{
    if (trackIndex < 0 || tracks == nullptr || trackIndex >= tracks->size())
        return {};
    if (!(trackIndex < automationExpanded.size() && automationExpanded.getUnchecked(trackIndex)))
        return {};

    const auto clipLane = getClipLaneBounds(trackIndex);
    const int timelineStartX = getTimelineStartX();
    const int timelineWidth = juce::jmax(0, getWidth() - timelineStartX);
    const int h = getTrackHeight(trackIndex);
    const int laneH = juce::jmax(16, (h / 4) - 4);
    const int top = clipLane.getY() + (h / 2) + 4;
    if (volumeLane)
        return { timelineStartX, top, timelineWidth, laneH };
    return { timelineStartX, top + laneH + 4, timelineWidth, laneH };
}

int64 ArrangementView::getClipPlaybackLengthSamples(const Track& track, const TrackClip& clip) const
{
    return track.getClipPlaybackLengthSamples(clip, sampleRate);
}

juce::Rectangle<int> ArrangementView::getClipRect(int trackIndex, int clipIndex) const
{
    if (tracks == nullptr || trackIndex < 0 || trackIndex >= tracks->size())
        return {};
    const auto& clips = tracks->getReference(trackIndex).getClips();
    if (clipIndex < 0 || clipIndex >= clips.size())
        return {};

    const auto& track = tracks->getReference(trackIndex);
    const auto& clip = clips.getReference(clipIndex);
    const int64 clipLengthSamples = getClipPlaybackLengthSamples(track, clip);
    const auto clipLaneBounds = getClipLaneBounds(trackIndex);
    const int x = sampleToXForTrack(trackIndex, clip.startSample);
    const int endX = sampleToXForTrack(trackIndex, clip.startSample + clipLengthSamples);
    const int w = juce::jmax(1, juce::jmin(endX, getWidth()) - x);
    return { x, clipLaneBounds.getY(), w, clipLaneBounds.getHeight() };
}

juce::Rectangle<int> ArrangementView::getClipGainHandleBounds(const juce::Rectangle<int>& clipRect) const
{
    return { clipRect.getCentreX() - 8, clipRect.getY() + 1, 16, clipRect.getHeight() - 2 };
}

juce::Rectangle<int> ArrangementView::getClipFadeInHandleBounds(const juce::Rectangle<int>& clipRect) const
{
    return { clipRect.getX() + 2, clipRect.getY() + 1, 10, 6 };
}

juce::Rectangle<int> ArrangementView::getClipFadeOutHandleBounds(const juce::Rectangle<int>& clipRect) const
{
    return { clipRect.getRight() - 12, clipRect.getY() + 1, 10, 6 };
}

juce::Rectangle<int> ArrangementView::getClipFitUnitsBounds(const juce::Rectangle<int>& clipRect) const
{
    return { clipRect.getRight() - 76, clipRect.getY() + 2, 34, 14 };
}

juce::Rectangle<int> ArrangementView::getClipFitModeBounds(const juce::Rectangle<int>& clipRect) const
{
    return { clipRect.getRight() - 40, clipRect.getY() + 2, 38, 14 };
}

juce::Rectangle<int> ArrangementView::getLeftNameFieldBounds(int trackIndex) const
{
    const int y = getTrackTop(trackIndex);
    const int h = getTrackHeight(trackIndex);
    auto strip = juce::Rectangle<int>(8, y + 8, trackControlsWidth - 16, h - 16);
    auto top = strip.removeFromTop(18);
    return top.reduced(3, 1);
}

juce::Rectangle<int> ArrangementView::getLeftTempoFieldBounds(int trackIndex) const
{
    const int y = getTrackTop(trackIndex);
    const int h = getTrackHeight(trackIndex);
    auto strip = juce::Rectangle<int>(8, y + 8, trackControlsWidth - 16, h - 16);
    auto bottom = strip.removeFromBottom(18).reduced(3, 1);
    return bottom.removeFromLeft(66);
}

juce::Rectangle<int> ArrangementView::getLeftTimeSigNumFieldBounds(int trackIndex) const
{
    const int y = getTrackTop(trackIndex);
    const int h = getTrackHeight(trackIndex);
    auto strip = juce::Rectangle<int>(8, y + 8, trackControlsWidth - 16, h - 16);
    auto bottom = strip.removeFromBottom(18).reduced(3, 1);
    bottom.removeFromLeft(72);
    return bottom.removeFromLeft(22);
}

juce::Rectangle<int> ArrangementView::getLeftTimeSigDenFieldBounds(int trackIndex) const
{
    const int y = getTrackTop(trackIndex);
    const int h = getTrackHeight(trackIndex);
    auto strip = juce::Rectangle<int>(8, y + 8, trackControlsWidth - 16, h - 16);
    auto bottom = strip.removeFromBottom(18).reduced(3, 1);
    bottom.removeFromLeft(97);
    return bottom.removeFromLeft(22);
}

juce::Rectangle<int> ArrangementView::getMixPadBounds(int trackIndex) const
{
    const int y = getTrackTop(trackIndex);
    const int h = getTrackHeight(trackIndex);
    const auto controlArea = juce::Rectangle<int>(8, y + 8, trackControlsWidth - 16, h - 16);
    const int padSize = juce::jmax(20, juce::jmin(controlArea.getWidth() - 10, controlArea.getHeight() - 10));
    const int padX = controlArea.getCentreX() - (padSize / 2);
    const int padY = controlArea.getCentreY() - (padSize / 2);
    return { padX, padY, padSize, padSize };
}

juce::Rectangle<int> ArrangementView::getZoomPadBounds(int trackIndex) const
{
    const int y = getTrackTop(trackIndex);
    const int h = getTrackHeight(trackIndex);
    const int timelineStartX = getTimelineStartX();
    const int timelineWidth = juce::jmax(0, getWidth() - timelineStartX);
    const int sliderWidth = juce::jlimit(60, 220, timelineWidth - 24);
    const int sliderHeight = 12;
    const int sliderX = juce::jmax(timelineStartX + 10, getWidth() - sliderWidth - 12);
    const int sliderY = y + h - sliderHeight - 10;
    return { sliderX, sliderY, sliderWidth, sliderHeight };
}

int ArrangementView::getDividerX(int dividerIndex) const
{
    if (dividerIndex == 0)
        return trackControlsWidth;
    if (dividerIndex == 1)
        return trackControlsWidth + trackHeaderWidth;
    return 0;
}

bool ArrangementView::hitColumnDivider(int x, int y, int& dividerIndexOut) const
{
    dividerIndexOut = -1;

    if (y < headerHeight)
        return false;

    const int d0 = getDividerX(0);
    const int d1 = getDividerX(1);
    const int hitRange = 5;

    if (std::abs(x - d0) <= hitRange)
    {
        dividerIndexOut = 0;
        return true;
    }
    if (std::abs(x - d1) <= hitRange)
    {
        dividerIndexOut = 1;
        return true;
    }
    return false;
}

void ArrangementView::setColumnWidthsFromDividerDrag(int dividerIndex, int mouseX)
{
    const int totalWidth = juce::jmax(1, getWidth());

    if (dividerIndex == 0)
    {
        const int maxControls = juce::jmax(minTrackControlsWidth, totalWidth - minTrackHeaderWidth - minTimelineWidth);
        const int oldSecondDivider = trackControlsWidth + trackHeaderWidth;
        trackControlsWidth = juce::jlimit(minTrackControlsWidth, maxControls, mouseX);

        const int minSecond = trackControlsWidth + minTrackHeaderWidth;
        const int maxSecond = juce::jmax(minSecond, totalWidth - minTimelineWidth);
        const int secondDivider = juce::jlimit(minSecond, maxSecond, oldSecondDivider);
        trackHeaderWidth = juce::jmax(minTrackHeaderWidth, secondDivider - trackControlsWidth);
    }
    else if (dividerIndex == 1)
    {
        const int minSecond = trackControlsWidth + minTrackHeaderWidth;
        const int maxSecond = juce::jmax(minSecond, totalWidth - minTimelineWidth);
        const int secondDivider = juce::jlimit(minSecond, maxSecond, mouseX);
        trackHeaderWidth = juce::jmax(minTrackHeaderWidth, secondDivider - trackControlsWidth);
    }

    layoutZoomSliders();
    if (layoutChanged)
        layoutChanged();
    repaint();
}

bool ArrangementView::hitTrackHeightHandle(int x, int y, int& trackIndexOut) const
{
    trackIndexOut = -1;
    if (tracks == nullptr || y < headerHeight || x < 0 || x > getWidth())
        return false;

    constexpr int handleRange = 4;
    for (int i = 0; i < tracks->size(); ++i)
    {
        const int boundaryY = getTrackTop(i) + getTrackHeight(i) - 1;
        if (std::abs(y - boundaryY) <= handleRange)
        {
            trackIndexOut = i;
            return true;
        }
    }
    return false;
}

bool ArrangementView::hitAutomationPoint(int trackIndex, bool volumeLane, juce::Point<float> pos, int& pointIndexOut) const
{
    pointIndexOut = -1;
    if (tracks == nullptr || trackIndex < 0 || trackIndex >= tracks->size())
        return false;

    const auto lane = getAutomationLaneBounds(trackIndex, volumeLane);
    if (lane.isEmpty())
        return false;

    const auto& points = volumeLane ? tracks->getReference(trackIndex).getVolumeAutomation()
                                    : tracks->getReference(trackIndex).getPanAutomation();
    int best = 9999;
    for (int i = 0; i < points.size(); ++i)
    {
        const auto& p = points.getReference(i);
        const int px = sampleToXForTrack(trackIndex, p.samplePosition);
        const int py = automationValueToY(volumeLane, p.value, lane);
        const int d = std::abs((int)pos.x - px) + std::abs((int)pos.y - py);
        if (d <= 10 && d < best)
        {
            best = d;
            pointIndexOut = i;
        }
    }
    return pointIndexOut >= 0;
}

double ArrangementView::yToAutomationValue(bool volumeLane, int y, const juce::Rectangle<int>& laneBounds) const
{
    if (laneBounds.getHeight() <= 1)
        return volumeLane ? 1.0 : 0.0;

    const double norm = 1.0 - juce::jlimit(0.0, 1.0, (double)(y - laneBounds.getY()) / (double)laneBounds.getHeight());
    if (volumeLane)
        return juce::jmap(norm, 0.0, 2.0);
    return juce::jmap(norm, -1.0, 1.0);
}

int ArrangementView::automationValueToY(bool volumeLane, double value, const juce::Rectangle<int>& laneBounds) const
{
    if (laneBounds.getHeight() <= 1)
        return laneBounds.getCentreY();

    const double clamped = volumeLane ? juce::jlimit(0.0, 2.0, value) : juce::jlimit(-1.0, 1.0, value);
    const double norm = volumeLane ? juce::jmap(clamped, 0.0, 2.0, 0.0, 1.0)
                                   : juce::jmap(clamped, -1.0, 1.0, 0.0, 1.0);
    return laneBounds.getBottom() - (int)std::llround(norm * laneBounds.getHeight());
}

bool ArrangementView::hitLoopMarker(int trackIndex, int mouseX, int& markerIndexOut) const
{
    markerIndexOut = -1;
    if (tracks == nullptr || trackIndex < 0 || trackIndex >= tracks->size())
        return false;

    const auto& track = tracks->getReference(trackIndex);
    int bestDist = 9999;
    for (int m = 0; m < 2; ++m)
    {
        if (!track.hasLoopMarker(m))
            continue;
        const int x = sampleToXForTrack(trackIndex, track.getLoopMarker(m));
        const int d = std::abs(mouseX - x);
        if (d <= 6 && d < bestDist)
        {
            bestDist = d;
            markerIndexOut = m;
        }
    }
    return markerIndexOut >= 0;
}

void ArrangementView::startInlineEdit(int trackIndex, EditField field, const juce::Rectangle<int>& bounds, const juce::String& value)
{
    editingTrack = trackIndex;
    editingField = field;
    suppressInlineCommit = false;
    inlineEditor.setBounds(bounds);
    inlineEditor.setText(value, juce::dontSendNotification);
    inlineEditor.setJustification(field == EditField::name ? juce::Justification::centredLeft
                                                           : juce::Justification::centred);
    inlineEditor.selectAll();
    inlineEditor.setVisible(true);
    inlineEditor.grabKeyboardFocus();
}

void ArrangementView::finishInlineEdit()
{
    if (tracks == nullptr || editingTrack < 0 || editingTrack >= tracks->size())
        return;
    if (editingField == EditField::none)
        return;
    if (suppressInlineCommit)
        return;

    const auto& track = tracks->getReference(editingTrack);
    const juce::String text = inlineEditor.getText().trim();

    if (editingField == EditField::name)
    {
        const juce::String nextName = text.isNotEmpty() ? text : track.getName();
        if (trackNameEdited)
            trackNameEdited(editingTrack, nextName);
    }
    else if (editingField == EditField::tempo)
    {
        const double nextTempo = juce::jlimit(20.0, 320.0, text.getDoubleValue());
        if (trackTempoEdited)
            trackTempoEdited(editingTrack, nextTempo);
    }
    else if (editingField == EditField::timeSigNum)
    {
        const int nextNum = juce::jlimit(1, 32, text.getIntValue());
        if (trackTimeSigEdited)
            trackTimeSigEdited(editingTrack, nextNum, track.getTimeSigDenominator());
    }
    else if (editingField == EditField::timeSigDen)
    {
        int nextDen = text.getIntValue();
        if (!(nextDen == 1 || nextDen == 2 || nextDen == 4 || nextDen == 8 || nextDen == 16))
            nextDen = track.getTimeSigDenominator();
        if (trackTimeSigEdited)
            trackTimeSigEdited(editingTrack, track.getTimeSigNumerator(), nextDen);
    }
    suppressInlineCommit = true;
    inlineEditor.setVisible(false);
    editingField = EditField::none;
    editingTrack = -1;
}

int ArrangementView::findClipSelectionIndex(int trackIndex, int clipIndex) const
{
    for (int i = 0; i < selectedClips.size(); ++i)
    {
        const auto& c = selectedClips.getReference(i);
        if (c.trackIndex == trackIndex && c.clipIndex == clipIndex)
            return i;
    }
    return -1;
}

int ArrangementView::findMarkerSelectionIndex(int trackIndex, int markerIndex) const
{
    for (int i = 0; i < selectedMarkers.size(); ++i)
    {
        const auto& m = selectedMarkers.getReference(i);
        if (m.trackIndex == trackIndex && m.markerIndex == markerIndex)
            return i;
    }
    return -1;
}

bool ArrangementView::isTrackSelected(int trackIndex) const
{
    return selectedTracks.contains(trackIndex);
}

bool ArrangementView::isClipSelected(int trackIndex, int clipIndex) const
{
    return findClipSelectionIndex(trackIndex, clipIndex) >= 0;
}

bool ArrangementView::isMarkerSelected(int trackIndex, int markerIndex) const
{
    return findMarkerSelectionIndex(trackIndex, markerIndex) >= 0;
}

void ArrangementView::setSingleTrackSelection(int trackIndex)
{
    selectionType = SelectionType::track;
    selectedTracks.clear();
    selectedClips.clear();
    selectedMarkers.clear();
    if (trackIndex >= 0)
        selectedTracks.add(trackIndex);
    selectedTrack = trackIndex;
    selectedClip = -1;
}

void ArrangementView::setSingleClipSelection(int trackIndex, int clipIndex)
{
    selectionType = SelectionType::clip;
    selectedTracks.clear();
    selectedClips.clear();
    selectedMarkers.clear();
    if (trackIndex >= 0 && clipIndex >= 0)
        selectedClips.add({trackIndex, clipIndex});
    selectedTrack = trackIndex;
    selectedClip = clipIndex;
}

void ArrangementView::setSingleMarkerSelection(int trackIndex, int markerIndex)
{
    selectionType = SelectionType::marker;
    selectedTracks.clear();
    selectedClips.clear();
    selectedMarkers.clear();
    if (trackIndex >= 0 && markerIndex >= 0)
        selectedMarkers.add({trackIndex, markerIndex});
    selectedLoopMarkerTrack = trackIndex;
    selectedLoopMarkerIndex = markerIndex;
}

void ArrangementView::toggleTrackSelection(int trackIndex)
{
    if (selectionType != SelectionType::track)
    {
        setSingleTrackSelection(trackIndex);
        return;
    }

    if (isTrackSelected(trackIndex))
        selectedTracks.removeAllInstancesOf(trackIndex);
    else
        selectedTracks.add(trackIndex);

    if (selectedTracks.isEmpty())
    {
        selectionType = SelectionType::none;
        selectedTrack = -1;
    }
    else
    {
        selectedTrack = selectedTracks.getLast();
    }
    selectedClip = -1;
}

void ArrangementView::toggleClipSelection(int trackIndex, int clipIndex)
{
    if (selectionType != SelectionType::clip)
    {
        setSingleClipSelection(trackIndex, clipIndex);
        return;
    }

    const int idx = findClipSelectionIndex(trackIndex, clipIndex);
    if (idx >= 0)
        selectedClips.remove(idx);
    else
        selectedClips.add({trackIndex, clipIndex});

    if (selectedClips.isEmpty())
    {
        selectionType = SelectionType::none;
        selectedTrack = -1;
        selectedClip = -1;
    }
    else
    {
        const auto& c = selectedClips.getLast();
        selectedTrack = c.trackIndex;
        selectedClip = c.clipIndex;
    }
}

void ArrangementView::toggleMarkerSelection(int trackIndex, int markerIndex)
{
    if (selectionType != SelectionType::marker)
    {
        setSingleMarkerSelection(trackIndex, markerIndex);
        return;
    }

    const int idx = findMarkerSelectionIndex(trackIndex, markerIndex);
    if (idx >= 0)
        selectedMarkers.remove(idx);
    else
        selectedMarkers.add({trackIndex, markerIndex});

    if (selectedMarkers.isEmpty())
    {
        selectionType = SelectionType::none;
        selectedLoopMarkerTrack = -1;
        selectedLoopMarkerIndex = -1;
    }
    else
    {
        const auto& m = selectedMarkers.getLast();
        selectedLoopMarkerTrack = m.trackIndex;
        selectedLoopMarkerIndex = m.markerIndex;
    }
}

void ArrangementView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(24, 26, 32));

    const int width = getWidth();
    juce::ColourGradient rulerGradient(juce::Colour(66, 70, 79), 0.0f, 0.0f,
                                       juce::Colour(46, 50, 58), 0.0f, (float)headerHeight, false);
    g.setGradientFill(rulerGradient);
    g.fillRect(0, 0, width, headerHeight);
    g.setColour(juce::Colour(255, 255, 255).withAlpha(0.16f));
    g.drawLine(0.0f, (float)headerHeight - 0.5f, (float)width, (float)headerHeight - 0.5f, 1.0f);

    if (tracks == nullptr)
        return;

    const int trackCount = tracks->size();
    constexpr double referenceTempoBpm = 120.0;
    auto chooseSubdivisionStep = [this](int trackIndex, int64 beatSamples) -> int64
    {
        int64 step = juce::jmax<int64>(1, beatSamples / 8); // start at 32nd-note style density
        const int x0 = sampleToXForTrack(trackIndex, 0);
        while (step < beatSamples)
        {
            const int x1 = sampleToXForTrack(trackIndex, step);
            if (x1 - x0 >= 9)
                break;
            step *= 2;
        }
        return juce::jmax<int64>(1, step);
    };
    const int timelineStartXGlobal = getTimelineStartX();
    const int divider0 = getDividerX(0);
    const int divider1 = getDividerX(1);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(juce::Colour(225, 225, 225).withAlpha(0.7f));
    g.drawText("MIX", 6, 4, juce::jmax(0, divider0 - 12), 14, juce::Justification::centredLeft, false);
    g.drawText("TRACK", divider0 + 6, 4, juce::jmax(0, divider1 - divider0 - 12), 14, juce::Justification::centredLeft, false);
    g.drawText("TIMELINE", divider1 + 6, 4, juce::jmax(0, width - divider1 - 12), 14, juce::Justification::centredLeft, false);
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillRect(divider0 - 1, 0, 2, getHeight());
    g.fillRect(divider1 - 1, 0, 2, getHeight());

    if (selectedTrack >= 0 && selectedTrack < trackCount)
    {
        const auto& selected = tracks->getReference(selectedTrack);
        const int64 beatSamples = selected.getBeatLengthSamples(sampleRate);
        const int64 barSamples = selected.getBarLengthSamples(sampleRate);
        const int64 maxVisibleSamples = xToSampleForTrack(selectedTrack, (float)width);

        if (beatSamples > 0 && barSamples > 0)
        {
            const int64 subStep = chooseSubdivisionStep(selectedTrack, beatSamples);
            for (int64 s = 0; s <= maxVisibleSamples; s += subStep)
            {
                if ((s % beatSamples) == 0)
                    continue;
                const int x = sampleToXForTrack(selectedTrack, s);
                g.setColour(juce::Colour(240, 244, 255).withAlpha(0.16f));
                g.drawLine((float)x, (float)headerHeight - 8.0f, (float)x, (float)headerHeight, 1.0f);
            }

            for (int64 s = 0; s <= maxVisibleSamples; s += beatSamples)
            {
                const int x = sampleToXForTrack(selectedTrack, s);
                const bool barLine = (s % barSamples) == 0;
                g.setColour(barLine ? juce::Colour(240, 244, 255).withAlpha(0.56f)
                                    : juce::Colour(240, 244, 255).withAlpha(0.32f));
                g.drawLine((float)x, 0.0f, (float)x, (float)headerHeight, barLine ? 1.2f : 1.0f);
                if (barLine)
                {
                    const int barNum = (int)(s / barSamples) + 1;
                    g.setColour(juce::Colour(240, 244, 255).withAlpha(0.82f));
                    g.drawText(juce::String(barNum), x + 3, 3, 42, 12, juce::Justification::centredLeft, false);
                }
            }
        }
        g.setColour(juce::Colour(255, 255, 255).withAlpha(0.22f));
        g.drawText("RULER", timelineStartXGlobal + 6, 2, 50, 12, juce::Justification::centredLeft, false);
    }

    for (int i = 0; i < trackCount; ++i)
    {
        const auto& track = tracks->getReference(i);
        const bool focusedTrack = isTrackSelected(i) || (selectionType == SelectionType::clip && selectedTrack == i);
        const int y = getTrackTop(i);
        const int h = getTrackHeight(i);

        const double tempoFactor = (track.getTempoBpm() / referenceTempoBpm) * (4.0 / (double)track.getTimeSigDenominator());
        const int64 trackPlayheadRaw = juce::jmax<int64>(0, (int64)std::llround((double)playheadSample * tempoFactor));
        const int64 trackPlayheadSample = track.mapTransportSampleToTrackSample(trackPlayheadRaw);

        auto headerRect = getTrackHeaderBounds(i);
        const int timelineStartX = getTimelineStartX();
        const int timelineWidth = juce::jmax(0, width - timelineStartX);

        auto controlsRect = juce::Rectangle<int>(0, y, trackControlsWidth, h - 1);
        auto timelineRect = juce::Rectangle<int>(timelineStartX, y, timelineWidth, h - 1);

        g.setColour(selectedTrack == i ? juce::Colour(122, 128, 138) : juce::Colour(98, 104, 114));
        g.fillRect(controlsRect);
        g.setColour(selectedTrack == i ? juce::Colour(149, 156, 167) : juce::Colour(126, 132, 141));
        g.fillRect(headerRect);
        g.setColour(selectedTrack == i ? juce::Colour(44, 49, 57) : juce::Colour(34, 38, 45));
        g.fillRect(timelineRect);
        g.setColour(juce::Colours::black.withAlpha(0.50f));
        g.drawRect(controlsRect, 1);
        g.drawRect(headerRect, 1);
        g.drawRect(timelineRect, 1);

        if (!focusedTrack && selectedTrack >= 0)
        {
            g.setColour(juce::Colour(0xff000000).withAlpha(0.14f));
            g.fillRect(juce::Rectangle<int>(0, y, width, h - 1));
        }

        if (focusedTrack)
        {
            const auto fullRow = juce::Rectangle<int>(0, y, width, h - 1);
            g.setColour(juce::Colour(0xff9cd0ff).withAlpha(0.85f));
            g.fillRect(0, y, 4, h - 1);
            g.drawRect(fullRow, 2);
            g.setColour(juce::Colour(0xff9cd0ff).withAlpha(0.12f));
            g.fillRect(fullRow.reduced(2, 2));
        }

        auto nameRect = getNameFieldBounds(i);
        auto tempoRect = getTempoFieldBounds(i);
        auto numRect = getTimeSigNumFieldBounds(i);
        auto denRect = getTimeSigDenFieldBounds(i);
        auto automationButtonRect = getAutomationToggleBounds(i);
        g.setColour(juce::Colours::white.withAlpha(focusedTrack ? 0.12f : 0.10f));
        g.drawRoundedRectangle(nameRect.toFloat(), 4.0f, 1.0f);
        g.drawRoundedRectangle(tempoRect.toFloat(), 4.0f, 1.0f);
        g.drawRoundedRectangle(numRect.toFloat(), 4.0f, 1.0f);
        g.drawRoundedRectangle(denRect.toFloat(), 4.0f, 1.0f);
        g.drawRoundedRectangle(automationButtonRect.toFloat(), 4.0f, 1.0f);

        g.setColour(juce::Colours::white.withAlpha(focusedTrack ? 0.88f : 0.75f));
        g.drawText(track.getName(), nameRect.reduced(6, 0), juce::Justification::centredLeft, true);
        g.drawText(juce::String(track.getTempoBpm(), 1), tempoRect, juce::Justification::centred, false);
        g.drawText(juce::String(track.getTimeSigNumerator()), numRect, juce::Justification::centred, false);
        g.drawText(juce::String(track.getTimeSigDenominator()), denRect, juce::Justification::centred, false);
        g.setColour(juce::Colours::white.withAlpha(focusedTrack ? 0.6f : 0.45f));
        g.drawText("/", numRect.getRight(), numRect.getY(), denRect.getX() - numRect.getRight(), numRect.getHeight(), juce::Justification::centred, false);
        g.setColour(juce::Colours::white.withAlpha(0.75f));
        g.drawText((i < automationExpanded.size() && automationExpanded.getUnchecked(i)) ? "A-" : "A+",
                   automationButtonRect, juce::Justification::centred, false);

        const int64 beatSamples = track.getBeatLengthSamples(sampleRate);
        int bar = 1;
        int beat = 1;
        int step = 1;
        if (beatSamples > 0)
        {
            const int64 totalBeats = trackPlayheadSample / beatSamples;
            bar = (int)(totalBeats / juce::jmax(1, track.getTimeSigNumerator())) + 1;
            beat = (int)(totalBeats % juce::jmax(1, track.getTimeSigNumerator())) + 1;
            const int64 beatOffsetSamples = trackPlayheadSample % beatSamples;
            step = (int)((beatOffsetSamples * 4) / juce::jmax<int64>(1, beatSamples)) + 1;
            step = juce::jlimit(1, 4, step);
        }
        if (focusedTrack)
        {
            g.drawText(juce::String(bar) + ":" + juce::String(beat) + ":" + juce::String(step),
                       headerRect.removeFromBottom(14).reduced(8, 0),
                       juce::Justification::centredRight,
                       false);
        }

        if ((i % 2) == 1)
        {
            g.setColour(juce::Colour(255, 255, 255).withAlpha(0.02f));
            g.fillRect(timelineRect);
        }
        g.setColour(juce::Colours::white.withAlpha(0.40f));
        g.drawText("MIX", getMixPadBounds(i).getX(), getMixPadBounds(i).getY() - 10, getMixPadBounds(i).getWidth(), 10, juce::Justification::centred, false);

        const auto mixPad = getMixPadBounds(i);
        juce::ColourGradient mixGrad(juce::Colour(60, 63, 72), (float)mixPad.getX(), (float)mixPad.getY(),
                                     juce::Colour(46, 49, 57), (float)mixPad.getRight(), (float)mixPad.getBottom(), false);
        g.setGradientFill(mixGrad);
        g.fillRoundedRectangle(mixPad.toFloat(), 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.16f));
        g.drawRoundedRectangle(mixPad.toFloat(), 4.0f, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.drawLine((float)mixPad.getCentreX(), (float)mixPad.getY(), (float)mixPad.getCentreX(), (float)mixPad.getBottom(), 1.0f);
        g.drawLine((float)mixPad.getX(), (float)mixPad.getCentreY(), (float)mixPad.getRight(), (float)mixPad.getCentreY(), 1.0f);

        const float panNorm = (float)juce::jmap(track.getPan(), -1.0, 1.0, 0.0, 1.0);
        const float volNorm = (float)juce::jmap(track.getVolume(), 0.0, 2.0, 0.0, 1.0);
        const float knobX = mixPad.getX() + panNorm * (float)mixPad.getWidth();
        const float knobY = mixPad.getBottom() - volNorm * (float)mixPad.getHeight();
        g.setColour(juce::Colour(210, 230, 255).withAlpha(0.95f));
        g.fillEllipse(knobX - 4.0f, knobY - 4.0f, 8.0f, 8.0f);

        const auto zoomPad = getZoomPadBounds(i);
        g.setColour(juce::Colour(18, 20, 24).withAlpha(0.85f));
        g.fillRoundedRectangle(zoomPad.toFloat(), 3.0f);
        g.setColour(juce::Colours::white.withAlpha(0.24f));
        g.drawRoundedRectangle(zoomPad.toFloat(), 3.0f, 1.0f);
        const float zoomXNorm = (float)juce::jmap(track.getZoomX(), 0.25, 8.0, 0.0, 1.0);
        const float zoomKnobX = zoomPad.getX() + zoomXNorm * (float)zoomPad.getWidth();
        const float zoomKnobY = (float)zoomPad.getCentreY();
        g.setColour(juce::Colour(220, 228, 240).withAlpha(0.95f));
        g.fillEllipse(zoomKnobX - 5.0f, zoomKnobY - 5.0f, 10.0f, 10.0f);
        g.setColour(juce::Colours::white.withAlpha(0.55f));
        g.drawText("ZOOM X", zoomPad.getX() - 52, zoomPad.getY() - 1, 48, zoomPad.getHeight() + 2, juce::Justification::centredRight, false);

        const int64 maxVisibleSamples = xToSampleForTrack(i, (float)width);
        const int64 barSamples = track.getBarLengthSamples(sampleRate);
        if (beatSamples > 0 && barSamples > 0)
        {
            const int64 subStep = chooseSubdivisionStep(i, beatSamples);
            for (int64 gridSample = 0; gridSample <= maxVisibleSamples; gridSample += subStep)
            {
                const int x = sampleToXForTrack(i, gridSample);
                const bool isBarLine = (gridSample % barSamples) == 0;
                const bool isBeatLine = (gridSample % beatSamples) == 0;
                if (isBarLine)
                {
                    g.setColour(juce::Colours::white.withAlpha(0.32f));
                    g.drawLine((float)x, (float)(y + 1), (float)x, (float)(y + h - 2), 1.3f);
                }
                else if (isBeatLine)
                {
                    g.setColour(juce::Colours::white.withAlpha(focusedTrack ? 0.18f : 0.12f));
                    g.drawLine((float)x, (float)(y + 1), (float)x, (float)(y + h - 2), 1.0f);
                }
                else if (focusedTrack)
                {
                    g.setColour(juce::Colours::white.withAlpha(0.08f));
                    g.drawLine((float)x, (float)(y + 1), (float)x, (float)(y + h - 2), 1.0f);
                }
            }
        }

        const int playX = sampleToXForTrack(i, trackPlayheadSample);
        const int timelineRightX = width;
        if (playX >= timelineStartX && playX <= timelineRightX)
        {
            g.setColour(juce::Colour(250, 252, 255).withAlpha(0.92f));
            g.drawLine((float)playX, (float)(y + 2), (float)playX, (float)(y + h - 3), 2.0f);
        }

        if (track.hasLoopMarker(0))
        {
            const int xLoop = sampleToXForTrack(i, track.getLoopMarker(0));
            if (xLoop >= timelineStartX && xLoop <= timelineRightX)
            {
                const bool selected = isMarkerSelected(i, 0);
                g.setColour(selected ? juce::Colour(180, 240, 255) : juce::Colour(120, 220, 255).withAlpha(0.85f));
                g.drawLine((float)xLoop, (float)(y + 2), (float)xLoop, (float)(y + h - 3), selected ? 2.6f : 1.6f);
            }
        }

        if (track.hasLoopMarker(0) && track.hasLoopMarker(1))
        {
            const int x0 = sampleToXForTrack(i, track.getLoopMarker(0));
            const int x1 = sampleToXForTrack(i, track.getLoopMarker(1));
            const int left = juce::jmax(timelineStartX, juce::jmin(x0, x1));
            const int right = juce::jmin(timelineRightX, juce::jmax(x0, x1));
            if (right > left)
            {
                const int yLine = y + 8;
                g.setColour(juce::Colour(145, 215, 255).withAlpha(0.72f));
                g.drawLine((float)left, (float)yLine, (float)right, (float)yLine, 1.0f);
            }
        }

        if (track.hasLoopMarker(1))
        {
            const int xLoop = sampleToXForTrack(i, track.getLoopMarker(1));
            if (xLoop >= timelineStartX && xLoop <= timelineRightX)
            {
                const bool selected = isMarkerSelected(i, 1);
                g.setColour(selected ? juce::Colour(165, 220, 255) : juce::Colour(95, 185, 235).withAlpha(0.85f));
                g.drawLine((float)xLoop, (float)(y + 2), (float)xLoop, (float)(y + h - 3), selected ? 2.6f : 1.6f);
            }
        }

        const auto clipLaneBounds = getClipLaneBounds(i);
        const bool expanded = (i < automationExpanded.size() && automationExpanded.getUnchecked(i));
        if (expanded)
        {
            const auto volLane = getAutomationLaneBounds(i, true);
            const auto panLane = getAutomationLaneBounds(i, false);
            g.setColour(juce::Colour(32, 36, 44));
            g.fillRoundedRectangle(volLane.toFloat(), 4.0f);
            g.fillRoundedRectangle(panLane.toFloat(), 4.0f);
            g.setColour(juce::Colours::white.withAlpha(0.12f));
            g.drawRoundedRectangle(volLane.toFloat(), 4.0f, 1.0f);
            g.drawRoundedRectangle(panLane.toFloat(), 4.0f, 1.0f);
            g.setColour(juce::Colours::white.withAlpha(0.52f));
            g.drawText("Vol Automation", volLane.reduced(8, 0), juce::Justification::centredLeft, false);
            g.drawText("Pan Automation", panLane.reduced(8, 0), juce::Justification::centredLeft, false);

            const auto& volPoints = track.getVolumeAutomation();
            if (!volPoints.isEmpty())
            {
                juce::Path path;
                for (int p = 0; p < volPoints.size(); ++p)
                {
                    const auto& pt = volPoints.getReference(p);
                    const float px = (float)sampleToXForTrack(i, pt.samplePosition);
                    const float py = (float)automationValueToY(true, pt.value, volLane);
                    if (p == 0) path.startNewSubPath(px, py);
                    else path.lineTo(px, py);
                }
                g.setColour(juce::Colour(120, 220, 140));
                g.strokePath(path, juce::PathStrokeType(2.0f));
                for (int p = 0; p < volPoints.size(); ++p)
                {
                    const auto& pt = volPoints.getReference(p);
                    const float px = (float)sampleToXForTrack(i, pt.samplePosition);
                    const float py = (float)automationValueToY(true, pt.value, volLane);
                    const bool selected = (selectedAutomationTrack == i && selectedAutomationVolumeLane
                                           && selectedAutomationPointIndex == p);
                    g.setColour(selected ? juce::Colour(250, 240, 140) : juce::Colour(180, 245, 190));
                    g.fillEllipse(px - 3.5f, py - 3.5f, 7.0f, 7.0f);
                }
            }

            const auto& panPoints = track.getPanAutomation();
            if (!panPoints.isEmpty())
            {
                juce::Path path;
                for (int p = 0; p < panPoints.size(); ++p)
                {
                    const auto& pt = panPoints.getReference(p);
                    const float px = (float)sampleToXForTrack(i, pt.samplePosition);
                    const float py = (float)automationValueToY(false, pt.value, panLane);
                    if (p == 0) path.startNewSubPath(px, py);
                    else path.lineTo(px, py);
                }
                g.setColour(juce::Colour(120, 180, 245));
                g.strokePath(path, juce::PathStrokeType(2.0f));
                for (int p = 0; p < panPoints.size(); ++p)
                {
                    const auto& pt = panPoints.getReference(p);
                    const float px = (float)sampleToXForTrack(i, pt.samplePosition);
                    const float py = (float)automationValueToY(false, pt.value, panLane);
                    const bool selected = (selectedAutomationTrack == i && !selectedAutomationVolumeLane
                                           && selectedAutomationPointIndex == p);
                    g.setColour(selected ? juce::Colour(250, 240, 140) : juce::Colour(180, 220, 255));
                    g.fillEllipse(px - 3.5f, py - 3.5f, 7.0f, 7.0f);
                }
            }
        }

            const auto& clips = track.getClips();
        for (int c = 0; c < clips.size(); ++c)
        {
            const auto& clip = clips.getReference(c);
            const auto clipRectI = getClipRect(i, c);
            const int x = clipRectI.getX();
            const int w = clipRectI.getWidth();
            const juce::Rectangle<float> clipRect = clipRectI.toFloat();

            juce::ColourGradient clipGrad(juce::Colour(93, 170, 132), clipRect.getX(), clipRect.getY(),
                                          juce::Colour(56, 118, 89), clipRect.getX(), clipRect.getBottom(), false);
            g.setGradientFill(clipGrad);
            g.fillRoundedRectangle(clipRect, 5.0f);
            g.setColour(juce::Colour(255, 255, 255).withAlpha(0.20f));
            g.drawRoundedRectangle(clipRect, 5.0f, 1.0f);

            // Draw waveform preview inside clip.
            if (clip.sample != nullptr && clip.sample->getNumSamples() > 0 && clipRect.getWidth() > 12.0f && clipRect.getHeight() > 16.0f)
            {
                const auto waveRect = clipRect.reduced(3.0f, 3.0f);
                const int waveW = juce::jmax(1, (int)waveRect.getWidth());
                const float midY = waveRect.getCentreY();
                const float maxHalf = waveRect.getHeight() * 0.49f;
                const int sampleChannels = juce::jmax(1, clip.sample->getNumChannels());

                juce::Graphics::ScopedSaveState ss(g);
                g.reduceClipRegion(waveRect.toNearestInt());
                g.setColour(juce::Colours::white.withAlpha(0.14f));
                g.drawLine(waveRect.getX(), midY, waveRect.getRight(), midY, 1.0f);

                const double srcStart = juce::jlimit(0.0, 1.0, (double)clip.sourceStartNorm);
                const double srcEnd = juce::jlimit(srcStart, 1.0, (double)clip.sourceEndNorm);
                const int analysisSteps = 256;
                float analysisPeak = 0.001f;
                for (int a = 0; a < analysisSteps; ++a)
                {
                    const double n = (double)a / (double)juce::jmax(1, analysisSteps - 1);
                    const double srcNorm = juce::jmap(n, srcStart, srcEnd);
                    const double samplePos = srcNorm * (double)(clip.sample->getNumSamples() - 1);
                    float v = 0.0f;
                    for (int ch = 0; ch < sampleChannels; ++ch)
                        v = juce::jmax(v, std::abs(clip.sample->getSampleAt(ch, samplePos)));
                    analysisPeak = juce::jmax(analysisPeak, v);
                }
                const float previewGain = juce::jlimit(1.0f, 8.0f, 0.95f / analysisPeak);

                g.setColour(juce::Colours::white.withAlpha(0.48f));
                for (int px = 0; px < waveW; ++px)
                {
                    const double n = (double)px / (double)juce::jmax(1, waveW - 1);
                    const double srcNorm = juce::jmap(n, srcStart, srcEnd);
                    const double samplePos = srcNorm * (double)(clip.sample->getNumSamples() - 1);
                    float s = 0.0f;
                    for (int ch = 0; ch < sampleChannels; ++ch)
                        s = juce::jmax(s, std::abs(clip.sample->getSampleAt(ch, samplePos)));
                    const float amp = juce::jlimit(0.0f, maxHalf, s * previewGain * maxHalf);
                    const float drawX = waveRect.getX() + (float)px;
                    g.drawLine(drawX, midY - amp, drawX, midY + amp);
                }

                // Draw beat-slice boundaries if present.
                if (!clip.slicing.slices.isEmpty())
                {
                    g.setColour(juce::Colour(255, 220, 130).withAlpha(0.85f));
                    const double srcRange = juce::jmax(0.000001, srcEnd - srcStart);
                    for (int s = 1; s < clip.slicing.slices.size(); ++s)
                    {
                        const auto& slice = clip.slicing.slices.getReference(s);
                        const double clipNorm = juce::jlimit(0.0, 1.0, slice.startNorm);
                        const double viewNorm = juce::jlimit(0.0, 1.0, (clipNorm - srcStart) / srcRange);
                        const float sx = waveRect.getX() + (float)viewNorm * waveRect.getWidth();
                        g.drawLine(sx, waveRect.getY(), sx, waveRect.getBottom(), 1.6f);
                    }
                }
            }

            const float fadeInPx = clipRect.getWidth() * juce::jlimit(0.0f, 0.98f, clip.fadeInNorm);
            const float fadeOutPx = clipRect.getWidth() * juce::jlimit(0.0f, 0.98f, clip.fadeOutNorm);
            if (fadeInPx > 1.0f)
            {
                juce::Path p;
                p.startNewSubPath(clipRect.getX(), clipRect.getBottom());
                p.lineTo(clipRect.getX() + fadeInPx, clipRect.getBottom());
                p.lineTo(clipRect.getX() + fadeInPx, clipRect.getY());
                p.closeSubPath();
                g.setColour(juce::Colours::black.withAlpha(0.12f));
                g.fillPath(p);
                g.setColour(juce::Colour(235, 245, 255).withAlpha(0.75f));
                g.drawLine(clipRect.getX(), clipRect.getBottom(), clipRect.getX() + fadeInPx, clipRect.getY(), 1.5f);
            }
            if (fadeOutPx > 1.0f)
            {
                juce::Path p;
                p.startNewSubPath(clipRect.getRight(), clipRect.getBottom());
                p.lineTo(clipRect.getRight() - fadeOutPx, clipRect.getBottom());
                p.lineTo(clipRect.getRight() - fadeOutPx, clipRect.getY());
                p.closeSubPath();
                g.setColour(juce::Colours::black.withAlpha(0.12f));
                g.fillPath(p);
                g.setColour(juce::Colour(235, 245, 255).withAlpha(0.75f));
                g.drawLine(clipRect.getRight() - fadeOutPx, clipRect.getY(), clipRect.getRight(), clipRect.getBottom(), 1.5f);
            }

            if (isClipSelected(i, c))
            {
                g.setColour(juce::Colour(146, 198, 255));
                g.drawRoundedRectangle(clipRect, 5.0f, 2.0f);
            }

            g.setColour(juce::Colour(12, 20, 14).withAlpha(0.75f));
            const int labelH = juce::jmin(16, clipLaneBounds.getHeight());
            const auto labelRect = juce::Rectangle<int>(x + 2, clipLaneBounds.getY() + 2, juce::jmax(1, w - 4), juce::jmax(1, labelH - 2));
            g.setColour(juce::Colour(8, 12, 10).withAlpha(0.40f));
            g.fillRoundedRectangle(labelRect.toFloat(), 3.0f);
            g.setColour(juce::Colours::white.withAlpha(0.16f));
            g.drawRoundedRectangle(labelRect.toFloat(), 3.0f, 1.0f);
            g.setColour(juce::Colour(230, 238, 232).withAlpha(0.92f));
            g.drawText(clip.name, labelRect.reduced(5, 0), juce::Justification::centredLeft, true);

            if (clipRectI.getWidth() >= 88)
            {
                const juce::String fitText = juce::String(juce::jmax(1, clip.fitLengthUnits))
                                           + "x "
                                           + (clip.fitToSnapDivision ? "Snap" : "Bars");
                g.setColour(juce::Colours::white.withAlpha(0.72f));
                g.setFont(juce::FontOptions(9.5f));
                g.drawText(fitText, labelRect.reduced(5, 0), juce::Justification::centredRight, false);
            }

            const auto gainHandle = getClipGainHandleBounds(clipRectI);
            const auto fadeInHandle = getClipFadeInHandleBounds(clipRectI);
            const auto fadeOutHandle = getClipFadeOutHandleBounds(clipRectI);
            const float gainNorm = juce::jlimit(0.0f, 1.0f, clip.gain / 2.0f);
            const float gainCx = (float)gainHandle.getCentreX();
            const float gainTravel = (float)juce::jmax(8, gainHandle.getHeight() - 8);
            const float gainCy = (float)gainHandle.getBottom() - 4.0f - gainNorm * gainTravel;
            const float gainDotSize = 7.0f;
            g.setColour(juce::Colour(255, 170, 64).withAlpha(0.96f));
            g.fillEllipse(gainCx - gainDotSize * 0.5f, gainCy - gainDotSize * 0.5f, gainDotSize, gainDotSize);
            g.setColour(juce::Colour(255, 220, 170).withAlpha(0.9f));
            g.drawEllipse(gainCx - gainDotSize * 0.5f, gainCy - gainDotSize * 0.5f, gainDotSize, gainDotSize, 1.0f);
            if (draggingClipGain && editClipTrack == i && editClipIndex == c)
            {
                const juce::String gainText = juce::String(clip.gain, 2) + "x";
                g.setColour(juce::Colours::black.withAlpha(0.55f));
                g.fillRoundedRectangle(gainCx + 8.0f, gainCy - 8.0f, 34.0f, 14.0f, 3.0f);
                g.setColour(juce::Colour(245, 250, 255).withAlpha(0.95f));
                g.drawText(gainText, (int)gainCx + 10, (int)gainCy - 8, 30, 14, juce::Justification::centredLeft, false);
            }
            const float dotSize = 6.0f;
            const float inCx = clipRect.getX() + juce::jlimit(0.0f, clipRect.getWidth() - 1.0f, fadeInPx);
            const float inCy = (float)fadeInHandle.getCentreY();
            const float outCx = clipRect.getRight() - juce::jlimit(0.0f, clipRect.getWidth() - 1.0f, fadeOutPx);
            const float outCy = (float)fadeOutHandle.getCentreY();
            g.setColour(juce::Colour(255, 170, 64).withAlpha(0.95f));
            g.fillEllipse(inCx - dotSize * 0.5f, inCy - dotSize * 0.5f, dotSize, dotSize);
            g.fillEllipse(outCx - dotSize * 0.5f, outCy - dotSize * 0.5f, dotSize, dotSize);
            g.setColour(juce::Colour(255, 220, 170).withAlpha(0.9f));
            g.drawEllipse(inCx - dotSize * 0.5f, inCy - dotSize * 0.5f, dotSize, dotSize, 1.0f);
            g.drawEllipse(outCx - dotSize * 0.5f, outCy - dotSize * 0.5f, dotSize, dotSize, 1.0f);
            const double clipSec = sampleRate > 0.0 ? (double)getClipPlaybackLengthSamples(track, clip) / sampleRate : 0.0;
            if (draggingClipFadeIn && editClipTrack == i && editClipIndex == c)
            {
                const juce::String fadeText = "In " + juce::String(clip.fadeInNorm * clipSec, 2) + "s";
                g.setColour(juce::Colours::black.withAlpha(0.55f));
                g.fillRoundedRectangle(inCx + 8.0f, inCy - 8.0f, 52.0f, 14.0f, 3.0f);
                g.setColour(juce::Colour(245, 250, 255).withAlpha(0.95f));
                g.drawText(fadeText, (int)inCx + 10, (int)inCy - 8, 48, 14, juce::Justification::centredLeft, false);
            }
            if (draggingClipFadeOut && editClipTrack == i && editClipIndex == c)
            {
                const juce::String fadeText = "Out " + juce::String(clip.fadeOutNorm * clipSec, 2) + "s";
                g.setColour(juce::Colours::black.withAlpha(0.55f));
                g.fillRoundedRectangle(outCx - 60.0f, outCy - 8.0f, 52.0f, 14.0f, 3.0f);
                g.setColour(juce::Colour(245, 250, 255).withAlpha(0.95f));
                g.drawText(fadeText, (int)outCx - 58, (int)outCy - 8, 48, 14, juce::Justification::centredLeft, false);
            }
        }
    }

    if (draggingClip && dragTargetTrack >= 0 && dragTargetTrack < trackCount && dragLengthSamples > 0)
    {
        const auto clipLaneBounds = getClipLaneBounds(dragTargetTrack);
        const int x = sampleToXForTrack(dragTargetTrack, dragStartSample);
        const int endX = sampleToXForTrack(dragTargetTrack, dragStartSample + dragLengthSamples);
        const int w = juce::jmax(1, juce::jmin(endX, getWidth()) - x);

        g.setColour(juce::Colour(230, 200, 120).withAlpha(0.25f));
        g.fillRoundedRectangle((float)x, (float)clipLaneBounds.getY(), (float)w, (float)clipLaneBounds.getHeight(), 6.0f);
        g.setColour(juce::Colour(240, 220, 120).withAlpha(0.85f));
        g.drawRoundedRectangle((float)x, (float)clipLaneBounds.getY(), (float)w, (float)clipLaneBounds.getHeight(), 6.0f, 2.0f);
    }

    if (editTool == EditTool::scissors && lastMouseX >= getTimelineStartX() && lastMouseY >= headerHeight)
    {
        const int t = yToTrackIndex((float)lastMouseY);
        if (t >= 0 && t < trackCount)
        {
            const auto lane = getClipLaneBounds(t);
            g.setColour(juce::Colour(255, 170, 80).withAlpha(0.9f));
            g.drawLine((float)lastMouseX, (float)lane.getY(), (float)lastMouseX, (float)lane.getBottom(), 1.4f);
        }
    }
}

void ArrangementView::resized()
{
    const int totalWidth = juce::jmax(1, getWidth());
    const int maxControls = juce::jmax(minTrackControlsWidth, totalWidth - minTrackHeaderWidth - minTimelineWidth);
    trackControlsWidth = juce::jlimit(minTrackControlsWidth, maxControls, trackControlsWidth);
    const int maxHeader = juce::jmax(minTrackHeaderWidth, totalWidth - trackControlsWidth - minTimelineWidth);
    trackHeaderWidth = juce::jlimit(minTrackHeaderWidth, maxHeader, trackHeaderWidth);

    layoutZoomSliders();

    if (!inlineEditor.isVisible())
        return;

    suppressInlineCommit = true;
    inlineEditor.setVisible(false);
    editingField = EditField::none;
    editingTrack = -1;
    suppressInlineCommit = false;
}

void ArrangementView::mouseDown(const juce::MouseEvent& e)
{
    if (tracks == nullptr)
        return;
    lastMouseX = (int)e.position.x;
    lastMouseY = (int)e.position.y;

    finishInlineEdit();

    if (canDragPlayhead == nullptr || canDragPlayhead())
    {
        auto dragToPosition = [this](int trackIdx, float xPos)
        {
            if (tracks == nullptr || trackIdx < 0 || trackIdx >= tracks->size())
                return;
            const auto& t = tracks->getReference(trackIdx);
            const double tempoFactor = (t.getTempoBpm() / 120.0) * (4.0 / (double)t.getTimeSigDenominator());
            const int64 trackSample = xToSampleForTrack(trackIdx, xPos);
            const int64 transportSample = (int64)std::llround((double)trackSample / juce::jmax(0.000001, tempoFactor));
            if (playheadDragged)
                playheadDragged(juce::jmax<int64>(0, transportSample));
        };

        const int refTrack = (selectedTrack >= 0 && selectedTrack < tracks->size()) ? selectedTrack : 0;
        if (e.position.y < (float)headerHeight)
        {
            draggingPlayhead = true;
            playheadDragTrack = refTrack;
            dragToPosition(playheadDragTrack, e.position.x);
            return;
        }

        for (int i = 0; i < tracks->size(); ++i)
        {
            const auto& t = tracks->getReference(i);
            const double tempoFactor = (t.getTempoBpm() / 120.0) * (4.0 / (double)t.getTimeSigDenominator());
            const int64 trackPlayheadRaw = juce::jmax<int64>(0, (int64)std::llround((double)playheadSample * tempoFactor));
            const int64 trackPlayheadSample = t.mapTransportSampleToTrackSample(trackPlayheadRaw);
            const int playX = sampleToXForTrack(i, trackPlayheadSample);
            const int y = getTrackTop(i);
            const int h = getTrackHeight(i);
            if ((int)e.position.y >= y && (int)e.position.y < y + h && std::abs((int)e.position.x - playX) <= 6)
            {
                draggingPlayhead = true;
                playheadDragTrack = i;
                dragToPosition(playheadDragTrack, e.position.x);
                return;
            }
        }
    }

    int dividerIndex = -1;
    if (hitColumnDivider((int)e.position.x, (int)e.position.y, dividerIndex))
    {
        draggingColumnDivider = true;
        draggingDividerIndex = dividerIndex;
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        return;
    }

    int heightTrackIndex = -1;
    if (hitTrackHeightHandle((int)e.position.x, (int)e.position.y, heightTrackIndex))
    {
        draggingTrackHeight = true;
        draggingTrackHeightTrack = heightTrackIndex;
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        return;
    }

    const int trackIndex = yToTrackIndex(e.position.y);
    if (trackIndex < 0)
        return;

    const bool shiftSelect = e.mods.isShiftDown();
    const bool wasFocusedTrack = isTrackSelected(trackIndex) || selectedTrack == trackIndex;
    if (!shiftSelect)
    {
        setSingleTrackSelection(trackIndex);
        if (trackSelected)
            trackSelected(trackIndex);
    }
    else
    {
        selectedTrack = trackIndex;
        selectedClip = -1;
    }
    draggingLoopMarker = false;
    draggingAutomationPoint = false;
    grabKeyboardFocus();
    layoutZoomSliders();

    const auto zoomPad = getZoomPadBounds(trackIndex);
    if (zoomPad.contains((int)e.position.x, (int)e.position.y))
    {
        const int cx = juce::jlimit(zoomPad.getX(), zoomPad.getRight(), (int)e.position.x);
        const double zx = juce::jmap((double)(cx - zoomPad.getX()) / (double)juce::jmax(1, zoomPad.getWidth()), 0.25, 8.0);
        if (trackZoomXEdited)
            trackZoomXEdited(trackIndex, zx);
        draggingZoomPad = true;
        draggingZoomPadTrack = trackIndex;
        repaint();
        return;
    }

    const int timelineStartX = getTimelineStartX();
    const int timelineRightX = getWidth();
    if ((int)e.position.x >= timelineStartX && (int)e.position.x < timelineRightX)
    {
        const auto volLane = getAutomationLaneBounds(trackIndex, true);
        const auto panLane = getAutomationLaneBounds(trackIndex, false);
        const bool inVolLane = !volLane.isEmpty() && volLane.contains((int)e.position.x, (int)e.position.y);
        const bool inPanLane = !panLane.isEmpty() && panLane.contains((int)e.position.x, (int)e.position.y);
        if (inVolLane || inPanLane)
        {
            const bool volumeLane = inVolLane;
            int hitIndex = -1;
            const bool hitPoint = hitAutomationPoint(trackIndex, volumeLane, e.position, hitIndex);
            auto points = volumeLane ? tracks->getReference(trackIndex).getVolumeAutomation()
                                     : tracks->getReference(trackIndex).getPanAutomation();

            if (e.mods.isRightButtonDown() && hitPoint)
            {
                points.remove(hitIndex);
                if (trackAutomationEdited)
                    trackAutomationEdited(trackIndex, volumeLane, points);
                selectedAutomationTrack = -1;
                selectedAutomationPointIndex = -1;
                repaint();
                return;
            }

            if (!hitPoint)
            {
                TrackAutomationPoint p;
                const int64 rawSample = xToSampleForTrack(trackIndex, e.position.x);
                p.samplePosition = tracks->getReference(trackIndex).snapSampleToGrid(rawSample, sampleRate);
                p.value = yToAutomationValue(volumeLane, (int)e.position.y, volumeLane ? volLane : panLane);
                points.add(p);
                if (trackAutomationEdited)
                    trackAutomationEdited(trackIndex, volumeLane, points);

                points = volumeLane ? tracks->getReference(trackIndex).getVolumeAutomation()
                                    : tracks->getReference(trackIndex).getPanAutomation();
                for (int i = 0; i < points.size(); ++i)
                {
                    if (points.getReference(i).samplePosition == p.samplePosition
                        && std::abs(points.getReference(i).value - p.value) < 0.000001)
                    {
                        hitIndex = i;
                        break;
                    }
                }
            }

            if (hitIndex >= 0)
            {
                draggingAutomationPoint = !e.mods.isRightButtonDown();
                selectedAutomationTrack = trackIndex;
                selectedAutomationPointIndex = hitIndex;
                selectedAutomationVolumeLane = volumeLane;
                repaint();
                return;
            }
        }

        int markerHit = -1;
        if (hitLoopMarker(trackIndex, (int)e.position.x, markerHit))
        {
            if (shiftSelect)
                toggleMarkerSelection(trackIndex, markerHit);
            else
                setSingleMarkerSelection(trackIndex, markerHit);
            draggingLoopMarker = !e.mods.isRightButtonDown();
            repaint();
            return;
        }

        if (e.mods.isRightButtonDown())
        {
            const int64 pos = xToSampleForTrack(trackIndex, e.position.x);
            const auto& t = tracks->getReference(trackIndex);
            int markerIndex = 0;
            if (!t.hasLoopMarker(0))
                markerIndex = 0;
            else if (!t.hasLoopMarker(1))
                markerIndex = 1;
            else
                markerIndex = (std::llabs(t.getLoopMarker(0) - pos) <= std::llabs(t.getLoopMarker(1) - pos)) ? 0 : 1;

            if (trackLoopMarkerEdited)
                trackLoopMarkerEdited(trackIndex, markerIndex, pos, true);

            setSingleMarkerSelection(trackIndex, markerIndex);
            repaint();
            return;
        }
    }

    if ((int)e.position.x < trackControlsWidth)
    {
        const auto mixPad = getMixPadBounds(trackIndex);
        if (mixPad.contains((int)e.position.x, (int)e.position.y))
        {
            const double pan = juce::jmap((double)((int)e.position.x - mixPad.getX()) / (double)juce::jmax(1, mixPad.getWidth()), -1.0, 1.0);
            const double vol = juce::jmap(1.0 - (double)((int)e.position.y - mixPad.getY()) / (double)juce::jmax(1, mixPad.getHeight()), 0.0, 2.0);
            if (trackPanEdited)
                trackPanEdited(trackIndex, pan);
            if (trackVolumeEdited)
                trackVolumeEdited(trackIndex, vol);
            draggingMixPad = true;
            draggingMixPadTrack = trackIndex;
            repaint();
            return;
        }

        return;
    }

    if ((int)e.position.x >= trackControlsWidth && (int)e.position.x < (trackControlsWidth + trackHeaderWidth))
    {
        if (!wasFocusedTrack)
        {
            if (shiftSelect)
                toggleTrackSelection(trackIndex);
            else
                setSingleTrackSelection(trackIndex);
            if (trackSelected)
                trackSelected(trackIndex);
            repaint();
            return;
        }

        const auto& track = tracks->getReference(trackIndex);
        const int px = (int)e.position.x;
        const int py = (int)e.position.y;
        if (getAutomationToggleBounds(trackIndex).contains(px, py))
        {
            if (trackIndex >= 0 && trackIndex < automationExpanded.size())
            {
                automationExpanded.set(trackIndex, !automationExpanded.getUnchecked(trackIndex));
                layoutZoomSliders();
                if (layoutChanged)
                    layoutChanged();
            }
        }
        else if (getNameFieldBounds(trackIndex).contains(px, py))
            startInlineEdit(trackIndex, EditField::name, getNameFieldBounds(trackIndex), track.getName());
        else if (getTempoFieldBounds(trackIndex).contains(px, py))
            startInlineEdit(trackIndex, EditField::tempo, getTempoFieldBounds(trackIndex), juce::String(track.getTempoBpm(), 1));
        else if (getTimeSigNumFieldBounds(trackIndex).contains(px, py))
            startInlineEdit(trackIndex, EditField::timeSigNum, getTimeSigNumFieldBounds(trackIndex), juce::String(track.getTimeSigNumerator()));
        else if (getTimeSigDenFieldBounds(trackIndex).contains(px, py))
            startInlineEdit(trackIndex, EditField::timeSigDen, getTimeSigDenFieldBounds(trackIndex), juce::String(track.getTimeSigDenominator()));
        repaint();
        return;
    }

    const auto& clips = tracks->getReference(trackIndex).getClips();
    for (int c = 0; c < clips.size(); ++c)
    {
        juce::Rectangle<int> r = getClipRect(trackIndex, c);

        if (r.contains((int)e.position.x, (int)e.position.y))
        {
            if (editTool == EditTool::eraser)
            {
                setSingleClipSelection(trackIndex, c);
                if (clipSelected)
                    clipSelected(trackIndex, c);
                if (clipDeleteRequested)
                    clipDeleteRequested(trackIndex, c);
                repaint();
                return;
            }

            if (shiftSelect)
            {
                toggleClipSelection(trackIndex, c);
                draggingClip = false;
            }
            else
            {
                setSingleClipSelection(trackIndex, c);
                const auto& clip = clips.getReference(c);
                const int64 clipLenSamples = getClipPlaybackLengthSamples(tracks->getReference(trackIndex), clip);
                const int64 clipEnd = clip.startSample + clipLenSamples;
                const int edgePx = 7;
                const bool nearLeftEdge = std::abs((int)e.position.x - r.getX()) <= edgePx;
                const bool nearRightEdge = std::abs((int)e.position.x - r.getRight()) <= edgePx;

                if (editTool == EditTool::scissors)
                {
                    if (nearLeftEdge || nearRightEdge)
                    {
                        draggingClipTrimStart = nearLeftEdge;
                        draggingClipTrimEnd = nearRightEdge;
                        editClipTrack = trackIndex;
                        editClipIndex = c;
                        trimOriginalStartSample = clip.startSample;
                        trimOriginalLengthSamples = clipLenSamples;
                        trimOriginalSourceStartNorm = clip.sourceStartNorm;
                        trimOriginalSourceEndNorm = clip.sourceEndNorm;
                        draggingClip = false;
                    }
                    else
                    {
                        const int64 split = juce::jlimit<int64>(clip.startSample + 1, clipEnd - 1,
                                                                xToSampleForTrack(trackIndex, e.position.x));
                        if (clipSplitRequested)
                            clipSplitRequested(trackIndex, c, split);
                        if (clipSelected)
                            clipSelected(trackIndex, c);
                        repaint();
                        return;
                    }
                }
                else if (editTool == EditTool::glue)
                {
                    if (!shiftSelect)
                        setSingleClipSelection(trackIndex, c);
                    const auto sameTrack = [trackIndex](const juce::Array<ClipRef>& refs)
                    {
                        if (refs.size() < 2)
                            return false;
                        for (const auto& r : refs)
                            if (r.trackIndex != trackIndex)
                                return false;
                        return true;
                    };
                    if (sameTrack(selectedClips) && clipGlueRequested)
                    {
                        clipGlueRequested(selectedClips);
                        repaint();
                    }
                    if (clipSelected)
                        clipSelected(trackIndex, c);
                    repaint();
                    return;
                }

                const auto gainHandle = getClipGainHandleBounds(r);
                const auto fadeInHandle = getClipFadeInHandleBounds(r);
                const auto fadeOutHandle = getClipFadeOutHandleBounds(r);
                const float gainNorm = juce::jlimit(0.0f, 1.0f, clip.gain / 2.0f);
                const float gainCx = (float)gainHandle.getCentreX();
                const float gainTravel = (float)juce::jmax(8, gainHandle.getHeight() - 8);
                const float gainCy = (float)gainHandle.getBottom() - 4.0f - gainNorm * gainTravel;
                const bool hitGainDot = e.position.getDistanceFrom({gainCx, gainCy}) <= 8.0f;
                const float fadeInPx = (float)r.getWidth() * juce::jlimit(0.0f, 0.98f, clip.fadeInNorm);
                const float fadeOutPx = (float)r.getWidth() * juce::jlimit(0.0f, 0.98f, clip.fadeOutNorm);
                const float fadeInCx = (float)r.getX() + juce::jlimit(0.0f, (float)r.getWidth() - 1.0f, fadeInPx);
                const float fadeOutCx = (float)r.getRight() - juce::jlimit(0.0f, (float)r.getWidth() - 1.0f, fadeOutPx);
                const float fadeCy = (float)fadeInHandle.getCentreY();
                const bool hitFadeInDot = e.position.getDistanceFrom({fadeInCx, fadeCy}) <= 8.0f;
                const bool hitFadeOutDot = e.position.getDistanceFrom({fadeOutCx, fadeCy}) <= 8.0f;

                if (draggingClipTrimStart || draggingClipTrimEnd)
                {
                    // Trim drag is already active (scissors mode edge hit).
                }
                else if (hitGainDot)
                {
                    draggingClipGain = true;
                    editClipTrack = trackIndex;
                    editClipIndex = c;
                    draggingClip = false;
                }
                else if (hitFadeInDot)
                {
                    draggingClipFadeIn = true;
                    editClipTrack = trackIndex;
                    editClipIndex = c;
                    draggingClip = false;
                }
                else if (hitFadeOutDot)
                {
                    draggingClipFadeOut = true;
                    editClipTrack = trackIndex;
                    editClipIndex = c;
                    draggingClip = false;
                }
                else
                {
                    draggingClip = true;
                    dragSourceTrack = trackIndex;
                    dragClipIndex = c;
                    dragTargetTrack = trackIndex;
                    dragStartSample = clip.startSample;
                    dragOriginalStartSample = clip.startSample;
                    dragLengthSamples = getClipPlaybackLengthSamples(tracks->getReference(trackIndex), clip);
                    const int64 pointerSample = xToSampleForTrack(trackIndex, e.position.x);
                    dragPointerOffsetSamples = pointerSample - clip.startSample;
                    clipDragMoved = false;
                }
            }
            repaint();
            if (clipSelected)
                clipSelected(trackIndex, c);
            return;
        }
    }

    if (shiftSelect)
        toggleTrackSelection(trackIndex);
    else
        setSingleTrackSelection(trackIndex);
    if (trackSelected)
        trackSelected(trackIndex);
    repaint();
}

void ArrangementView::mouseDrag(const juce::MouseEvent& e)
{
    lastMouseX = (int)e.position.x;
    lastMouseY = (int)e.position.y;

    if (draggingPlayhead)
    {
        if (playheadDragTrack >= 0 && tracks != nullptr && playheadDragTrack < tracks->size())
        {
            const auto& t = tracks->getReference(playheadDragTrack);
            const double tempoFactor = (t.getTempoBpm() / 120.0) * (4.0 / (double)t.getTimeSigDenominator());
            const int64 trackSample = xToSampleForTrack(playheadDragTrack, e.position.x);
            const int64 transportSample = (int64)std::llround((double)trackSample / juce::jmax(0.000001, tempoFactor));
            if (playheadDragged)
                playheadDragged(juce::jmax<int64>(0, transportSample));
        }
        return;
    }

    if (draggingColumnDivider)
    {
        setColumnWidthsFromDividerDrag(draggingDividerIndex, (int)e.position.x);
        return;
    }

    if (draggingTrackHeight && draggingTrackHeightTrack >= 0 && tracks != nullptr && draggingTrackHeightTrack < tracks->size())
    {
        const int trackIndex = draggingTrackHeightTrack;
        const int top = getTrackTop(trackIndex);
        const bool expanded = (trackIndex < automationExpanded.size() && automationExpanded.getUnchecked(trackIndex));
        const int minTotalHeight = expanded ? 180 : 108;
        const int maxTotalHeight = expanded ? 720 : 360;
        const int desiredTotal = juce::jlimit(minTotalHeight, maxTotalHeight, (int)e.position.y - top);
        const int desiredMain = expanded ? juce::jmax(90, desiredTotal / 2) : juce::jmax(80, desiredTotal);
        const double zoomY = juce::jlimit(0.5, 3.0, (double)desiredMain / (double)trackHeight);

        if (trackZoomYEdited)
            trackZoomYEdited(trackIndex, zoomY);
        if (layoutChanged)
            layoutChanged();
        repaint();
        return;
    }

    if ((draggingClipTrimStart || draggingClipTrimEnd)
        && tracks != nullptr
        && editClipTrack >= 0 && editClipTrack < tracks->size())
    {
        const auto& clips = tracks->getReference(editClipTrack).getClips();
        if (editClipIndex < 0 || editClipIndex >= clips.size())
            return;

        const int64 minLen = juce::jmax<int64>(128, (int64)(sampleRate * 0.01));
        const int64 oldStart = trimOriginalStartSample;
        const int64 oldLen = juce::jmax<int64>(1, trimOriginalLengthSamples);
        const int64 oldEnd = oldStart + oldLen;
        const double oldSrcStart = juce::jlimit(0.0, 1.0, (double)trimOriginalSourceStartNorm);
        const double oldSrcEnd = juce::jlimit(oldSrcStart, 1.0, (double)trimOriginalSourceEndNorm);
        const double oldSrcRange = juce::jmax(0.000001, oldSrcEnd - oldSrcStart);

        int64 newStart = oldStart;
        int64 newLen = oldLen;
        double newSrcStart = oldSrcStart;
        double newSrcEnd = oldSrcEnd;

        if (draggingClipTrimStart)
        {
            const int64 pointer = xToSampleForTrack(editClipTrack, e.position.x);
            newStart = juce::jlimit<int64>(oldStart, oldEnd - minLen, pointer);
            const double f = (double)(newStart - oldStart) / (double)oldLen;
            newSrcStart = oldSrcStart + oldSrcRange * juce::jlimit(0.0, 1.0, f);
            newLen = juce::jmax<int64>(minLen, oldEnd - newStart);
        }
        else if (draggingClipTrimEnd)
        {
            const int64 pointer = xToSampleForTrack(editClipTrack, e.position.x);
            const int64 newEnd = juce::jlimit<int64>(oldStart + minLen, oldEnd, pointer);
            const double f = (double)(newEnd - oldStart) / (double)oldLen;
            newSrcEnd = oldSrcStart + oldSrcRange * juce::jlimit(0.0, 1.0, f);
            newLen = juce::jmax<int64>(minLen, newEnd - oldStart);
        }

        if (clipTrimRequested)
            clipTrimRequested(editClipTrack, editClipIndex, newStart, newLen, (float)newSrcStart, (float)newSrcEnd);
        repaint();
        return;
    }

    if ((draggingClipGain || draggingClipFadeIn || draggingClipFadeOut)
        && tracks != nullptr
        && editClipTrack >= 0 && editClipTrack < tracks->size())
    {
        const auto& clips = tracks->getReference(editClipTrack).getClips();
        if (editClipIndex < 0 || editClipIndex >= clips.size())
            return;
        const auto& clip = clips.getReference(editClipIndex);
        const auto clipRect = getClipRect(editClipTrack, editClipIndex);
        if (!clipRect.isEmpty())
        {
            if (draggingClipGain)
            {
                const double norm = 1.0 - (double)((int)e.position.y - clipRect.getY()) / (double)juce::jmax(1, clipRect.getHeight());
                const float g = (float)juce::jmap(juce::jlimit(0.0, 1.0, norm), 0.0, 2.0);
                if (clipGainEdited)
                    clipGainEdited(editClipTrack, editClipIndex, g);
            }
            else if (draggingClipFadeIn)
            {
                const double rel = ((double)e.position.x - (double)clipRect.getX()) / (double)juce::jmax(1, clipRect.getWidth());
                const float fadeIn = (float)juce::jlimit(0.0, 0.98 - (double)clip.fadeOutNorm, rel);
                if (clipFadeEdited)
                    clipFadeEdited(editClipTrack, editClipIndex, fadeIn, clip.fadeOutNorm);
            }
            else if (draggingClipFadeOut)
            {
                const double rel = ((double)clipRect.getRight() - (double)e.position.x) / (double)juce::jmax(1, clipRect.getWidth());
                const float fadeOut = (float)juce::jlimit(0.0, 0.98 - (double)clip.fadeInNorm, rel);
                if (clipFadeEdited)
                    clipFadeEdited(editClipTrack, editClipIndex, clip.fadeInNorm, fadeOut);
            }
        }
        repaint();
        return;
    }

    if (draggingZoomPad && draggingZoomPadTrack >= 0 && tracks != nullptr && draggingZoomPadTrack < tracks->size())
    {
        const auto zoomPad = getZoomPadBounds(draggingZoomPadTrack);
        const int cx = juce::jlimit(zoomPad.getX(), zoomPad.getRight(), (int)e.position.x);
        const double zx = juce::jmap((double)(cx - zoomPad.getX()) / (double)juce::jmax(1, zoomPad.getWidth()), 0.25, 8.0);
        if (trackZoomXEdited)
            trackZoomXEdited(draggingZoomPadTrack, zx);
        repaint();
        return;
    }

    if (draggingMixPad && draggingMixPadTrack >= 0 && tracks != nullptr && draggingMixPadTrack < tracks->size())
    {
        const auto mixPad = getMixPadBounds(draggingMixPadTrack);
        const int cx = juce::jlimit(mixPad.getX(), mixPad.getRight(), (int)e.position.x);
        const int cy = juce::jlimit(mixPad.getY(), mixPad.getBottom(), (int)e.position.y);
        const double pan = juce::jmap((double)(cx - mixPad.getX()) / (double)juce::jmax(1, mixPad.getWidth()), -1.0, 1.0);
        const double vol = juce::jmap(1.0 - (double)(cy - mixPad.getY()) / (double)juce::jmax(1, mixPad.getHeight()), 0.0, 2.0);
        if (trackPanEdited)
            trackPanEdited(draggingMixPadTrack, pan);
        if (trackVolumeEdited)
            trackVolumeEdited(draggingMixPadTrack, vol);
        repaint();
        return;
    }

    if (draggingAutomationPoint && selectedAutomationTrack >= 0 && selectedAutomationPointIndex >= 0)
    {
        if (tracks == nullptr || selectedAutomationTrack >= tracks->size())
            return;

        const bool volumeLane = selectedAutomationVolumeLane;
        auto laneBounds = getAutomationLaneBounds(selectedAutomationTrack, volumeLane);
        auto points = volumeLane ? tracks->getReference(selectedAutomationTrack).getVolumeAutomation()
                                 : tracks->getReference(selectedAutomationTrack).getPanAutomation();
        if (selectedAutomationPointIndex >= points.size())
            return;

        auto p = points.getReference(selectedAutomationPointIndex);
        const int64 rawSample = xToSampleForTrack(selectedAutomationTrack, e.position.x);
        p.samplePosition = tracks->getReference(selectedAutomationTrack).snapSampleToGrid(rawSample, sampleRate);
        p.value = yToAutomationValue(volumeLane, (int)e.position.y, laneBounds);
        points.set(selectedAutomationPointIndex, p);

        if (trackAutomationEdited)
            trackAutomationEdited(selectedAutomationTrack, volumeLane, points);

        const auto& updated = volumeLane ? tracks->getReference(selectedAutomationTrack).getVolumeAutomation()
                                         : tracks->getReference(selectedAutomationTrack).getPanAutomation();
        int bestIdx = -1;
        int64 bestDist = std::numeric_limits<int64>::max();
        for (int i = 0; i < updated.size(); ++i)
        {
            const int64 d = std::llabs(updated.getReference(i).samplePosition - p.samplePosition);
            if (d < bestDist)
            {
                bestDist = d;
                bestIdx = i;
            }
        }
        selectedAutomationPointIndex = bestIdx;
        repaint();
        return;
    }

    if (draggingLoopMarker && selectedLoopMarkerTrack >= 0 && selectedLoopMarkerIndex >= 0)
    {
        const int64 pos = xToSampleForTrack(selectedLoopMarkerTrack, e.position.x);
        if (trackLoopMarkerEdited)
            trackLoopMarkerEdited(selectedLoopMarkerTrack, selectedLoopMarkerIndex, pos, true);
        repaint();
        return;
    }

    if (!draggingClip || tracks == nullptr)
        return;
    if (dragSourceTrack < 0 || dragSourceTrack >= tracks->size())
        return;

    int targetTrack = yToTrackIndex(e.position.y);
    if (targetTrack < 0)
        targetTrack = tracks->size() - 1;

    int64 unsnappedStart = xToSampleForTrack(targetTrack, e.position.x) - dragPointerOffsetSamples;
    unsnappedStart = juce::jmax<int64>(0, unsnappedStart);

    const auto& target = tracks->getReference(targetTrack);
    const int64 snappedStart = target.snapSampleToGrid(unsnappedStart, sampleRate);

    dragTargetTrack = targetTrack;
    dragStartSample = snappedStart;
    if (targetTrack != dragSourceTrack || snappedStart != dragOriginalStartSample)
        clipDragMoved = true;
    repaint();
}

void ArrangementView::mouseUp(const juce::MouseEvent&)
{
    draggingPlayhead = false;
    playheadDragTrack = -1;
    draggingColumnDivider = false;
    draggingDividerIndex = -1;
    draggingTrackHeight = false;
    draggingTrackHeightTrack = -1;
    setMouseCursor(juce::MouseCursor::NormalCursor);

    draggingLoopMarker = false;
    draggingMixPad = false;
    draggingMixPadTrack = -1;
    draggingZoomPad = false;
    draggingZoomPadTrack = -1;
    draggingClipTrimStart = false;
    draggingClipTrimEnd = false;
    draggingClipGain = false;
    draggingClipFadeIn = false;
    draggingClipFadeOut = false;
    editClipTrack = -1;
    editClipIndex = -1;
    draggingAutomationPoint = false;

    if (!draggingClip)
        return;

    const int sourceTrack = dragSourceTrack;
    const int clipIndex = dragClipIndex;
    const int targetTrack = dragTargetTrack;
    const int64 newStart = dragStartSample;
    const bool moved = clipDragMoved;

    draggingClip = false;
    dragSourceTrack = -1;
    dragClipIndex = -1;
    dragTargetTrack = -1;
    dragOriginalStartSample = 0;
    dragLengthSamples = 0;
    dragPointerOffsetSamples = 0;
    clipDragMoved = false;
    repaint();

    if (!moved)
        return;

    if (clipMoved)
        clipMoved(sourceTrack, clipIndex, targetTrack, newStart);

    layoutZoomSliders();
}

void ArrangementView::mouseMove(const juce::MouseEvent& e)
{
    lastMouseX = (int)e.position.x;
    lastMouseY = (int)e.position.y;

    int dividerIndex = -1;
    if (hitColumnDivider((int)e.position.x, (int)e.position.y, dividerIndex))
    {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        return;
    }

    int heightTrackIndex = -1;
    if (hitTrackHeightHandle((int)e.position.x, (int)e.position.y, heightTrackIndex))
    {
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
        return;
    }

    if (editTool == EditTool::scissors && (int)e.position.x >= getTimelineStartX() && (int)e.position.y >= headerHeight)
    {
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
        repaint();
        return;
    }
    if ((editTool == EditTool::eraser || editTool == EditTool::glue)
        && (int)e.position.x >= getTimelineStartX() && (int)e.position.y >= headerHeight)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        repaint();
        return;
    }

    setMouseCursor(juce::MouseCursor::NormalCursor);
    if (editTool == EditTool::scissors)
        repaint();
}

bool ArrangementView::keyPressed(const juce::KeyPress& key)
{
    const auto mods = key.getModifiers();
    const int kc = key.getKeyCode();
    if (mods.isCommandDown() && (kc == 'z' || kc == 'Z'))
    {
        if (undoRequested)
            undoRequested();
        return true;
    }

    if (mods.isCommandDown() && (kc == 'c' || kc == 'C'))
    {
        if (copyRequested)
            copyRequested();
        return true;
    }

    if (mods.isCommandDown() && (kc == 'v' || kc == 'V'))
    {
        if (pasteRequested)
            pasteRequested();
        return true;
    }

    if ((key.getKeyCode() == juce::KeyPress::backspaceKey || key.getKeyCode() == juce::KeyPress::deleteKey)
        && selectedAutomationTrack >= 0 && selectedAutomationPointIndex >= 0 && tracks != nullptr
        && selectedAutomationTrack < tracks->size())
    {
        const bool volumeLane = selectedAutomationVolumeLane;
        auto points = volumeLane ? tracks->getReference(selectedAutomationTrack).getVolumeAutomation()
                                 : tracks->getReference(selectedAutomationTrack).getPanAutomation();
        if (selectedAutomationPointIndex < points.size())
        {
            points.remove(selectedAutomationPointIndex);
            if (trackAutomationEdited)
                trackAutomationEdited(selectedAutomationTrack, volumeLane, points);
        }
        selectedAutomationTrack = -1;
        selectedAutomationPointIndex = -1;
        repaint();
        return true;
    }

    if ((key.getKeyCode() == juce::KeyPress::backspaceKey || key.getKeyCode() == juce::KeyPress::deleteKey)
        && selectedLoopMarkerTrack >= 0 && selectedLoopMarkerIndex >= 0)
    {
        if (trackLoopMarkerEdited)
            trackLoopMarkerEdited(selectedLoopMarkerTrack, selectedLoopMarkerIndex, 0, false);
        selectedLoopMarkerTrack = -1;
        selectedLoopMarkerIndex = -1;
        repaint();
        return true;
    }

    if ((key.getKeyCode() == juce::KeyPress::backspaceKey || key.getKeyCode() == juce::KeyPress::deleteKey)
        && selectedTrack >= 0)
    {
        if (trackDeleteRequested)
            trackDeleteRequested(selectedTrack);
        return true;
    }

    return false;
}

void ArrangementView::ensureAutomationExpandedState()
{
    const int needed = tracks != nullptr ? tracks->size() : 0;
    while (automationExpanded.size() < needed)
        automationExpanded.add(false);
    while (automationExpanded.size() > needed)
        automationExpanded.removeLast();
}

void ArrangementView::ensureZoomSliders()
{
    // Zoom is now controlled by per-track XY pad (no dedicated sliders).
}

void ArrangementView::layoutZoomSliders()
{
    // Kept for compatibility with existing call sites.
}
