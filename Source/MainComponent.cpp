#include "MainComponent.h"
#include <cmath>
#include <limits>
#include <vector>

namespace
{
juce::var warpCurveToVar(const WarpCurve& curve)
{
    juce::Array<juce::var> arr;
    for (const auto& p : curve.getPoints())
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("t", p.t);
        obj->setProperty("v", p.v);
        arr.add(juce::var(obj));
    }

    auto* root = new juce::DynamicObject();
    root->setProperty("points", juce::var(arr));
    root->setProperty("relative", curve.isRelativeMode());
    root->setProperty("smoothRateChanges", curve.getSmoothRateChanges());
    return juce::var(root);
}

WarpCurve varToWarpCurve(const juce::var& v)
{
    WarpCurve curve = WarpCurve::linear();

    juce::Array<WarpCurve::Point> points;
    bool relative = true;
    bool smooth = false;
    const juce::Array<juce::var>* arr = nullptr;

    if (v.isArray())
    {
        arr = v.getArray();
    }
    else if (auto* obj = v.getDynamicObject())
    {
        if (obj->hasProperty("relative"))
            relative = (bool)obj->getProperty("relative");
        if (obj->hasProperty("smoothRateChanges"))
            smooth = (bool)obj->getProperty("smoothRateChanges");
        const auto pointsVar = obj->getProperty("points");
        if (pointsVar.isArray())
            arr = pointsVar.getArray();
    }

    if (arr == nullptr)
        return curve;

    for (const auto& item : *arr)
    {
        auto* obj = item.getDynamicObject();
        if (obj == nullptr)
            continue;

        WarpCurve::Point p;
        p.t = (double)obj->getProperty("t");
        p.v = (double)obj->getProperty("v");
        points.add(p);
    }

    if (!points.isEmpty())
        curve.setPoints(points);
    curve.setRelativeMode(relative);
    curve.setSmoothRateChanges(smooth);
    return curve;
}

juce::String sliceModeToString(SlicePlaybackMode mode)
{
    if (mode == SlicePlaybackMode::rotate) return "rotate";
    if (mode == SlicePlaybackMode::random) return "random";
    return "sequential";
}

SlicePlaybackMode stringToSliceMode(const juce::String& mode)
{
    if (mode == "rotate") return SlicePlaybackMode::rotate;
    if (mode == "random") return SlicePlaybackMode::random;
    return SlicePlaybackMode::sequential;
}

int snapDivisionToId(Track::SnapDivision division)
{
    switch (division)
    {
        case Track::SnapDivision::bar: return 1;
        case Track::SnapDivision::beat: return 2;
        case Track::SnapDivision::eighth: return 3;
        case Track::SnapDivision::sixteenth: return 4;
        case Track::SnapDivision::thirtySecond: return 5;
        case Track::SnapDivision::eighthTriplet: return 6;
        case Track::SnapDivision::sixteenthTriplet: return 7;
        default: return 4;
    }
}

Track::SnapDivision idToSnapDivision(int id)
{
    switch (id)
    {
        case 1: return Track::SnapDivision::bar;
        case 2: return Track::SnapDivision::beat;
        case 3: return Track::SnapDivision::eighth;
        case 4: return Track::SnapDivision::sixteenth;
        case 5: return Track::SnapDivision::thirtySecond;
        case 6: return Track::SnapDivision::eighthTriplet;
        case 7: return Track::SnapDivision::sixteenthTriplet;
        default: return Track::SnapDivision::sixteenth;
    }
}

void styleSectionLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text.toUpperCase(), juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setFont(juce::FontOptions(10.5f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, juce::Colour(205, 210, 220).withAlpha(0.72f));
}

class AudioSettingsPane : public juce::Component
{
public:
    AudioSettingsPane(juce::AudioDeviceManager& deviceManagerIn, int bitDepthIn, std::function<void(int)> onBitDepthChangedIn)
        : deviceSelector(deviceManagerIn, 0, 2, 0, 2, false, false, true, false),
          onBitDepthChanged(std::move(onBitDepthChangedIn))
    {
        addAndMakeVisible(deviceSelector);
        addAndMakeVisible(bitDepthLabel);
        addAndMakeVisible(bitDepthBox);
        addAndMakeVisible(systemDefaultHint);

        bitDepthLabel.setText("Export Bit Depth", juce::dontSendNotification);
        bitDepthLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.86f));
        bitDepthLabel.setJustificationType(juce::Justification::centredLeft);

        bitDepthBox.addItem("16-bit", 16);
        bitDepthBox.addItem("24-bit", 24);
        bitDepthBox.addItem("32-bit", 32);
        bitDepthBox.setSelectedId(bitDepthIn, juce::dontSendNotification);
        bitDepthBox.onChange = [this]()
        {
            if (onBitDepthChanged)
                onBitDepthChanged(bitDepthBox.getSelectedId());
        };

        systemDefaultHint.setText("Audio device defaults to system output unless changed here.", juce::dontSendNotification);
        systemDefaultHint.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.62f));
        systemDefaultHint.setJustificationType(juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        auto topRow = area.removeFromTop(28);
        bitDepthLabel.setBounds(topRow.removeFromLeft(140));
        bitDepthBox.setBounds(topRow.removeFromLeft(100));
        systemDefaultHint.setBounds(area.removeFromTop(22));
        area.removeFromTop(6);
        deviceSelector.setBounds(area);
    }

private:
    juce::AudioDeviceSelectorComponent deviceSelector;
    juce::Label bitDepthLabel;
    juce::ComboBox bitDepthBox;
    juce::Label systemDefaultHint;
    std::function<void(int)> onBitDepthChanged;
};
}

MainComponent::MainComponent()
{
    setSize(1200, 720);

    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(scissorsButton);
    addAndMakeVisible(eraserButton);
    addAndMakeVisible(glueButton);
    addAndMakeVisible(loadButton);
    addAndMakeVisible(exportButton);
    addAndMakeVisible(projectButton);
    addAndMakeVisible(addTrackButton);
    addAndMakeVisible(settingsButton);
    addAndMakeVisible(snapToggle);
    addAndMakeVisible(snapDivisionBox);
    addAndMakeVisible(transportSectionLabel);
    addAndMakeVisible(mediaSectionLabel);
    addAndMakeVisible(projectSectionLabel);
    addAndMakeVisible(trackSectionLabel);
    addAndMakeVisible(arrangementViewport);
    addAndMakeVisible(curveEditor);
    addAndMakeVisible(warpPanel);
    addAndMakeVisible(beatSlicer);
    beatSlicer.setVisible(false);
    arrangementViewport.setViewedComponent(&arrangementView, false);
    arrangementViewport.setScrollBarsShown(true, false);

    playButton.addListener(this);
    stopButton.addListener(this);
    scissorsButton.addListener(this);
    eraserButton.addListener(this);
    glueButton.addListener(this);
    loadButton.addListener(this);
    exportButton.addListener(this);
    projectButton.addListener(this);
    addTrackButton.addListener(this);
    settingsButton.addListener(this);

    styleSectionLabel(transportSectionLabel, "Transport");
    styleSectionLabel(mediaSectionLabel, "Media");
    styleSectionLabel(projectSectionLabel, "Project");
    styleSectionLabel(trackSectionLabel, "Track");

    styleToolbarButton(playButton, juce::Colour(50, 118, 215), true);
    styleToolbarButton(stopButton, juce::Colour(145, 66, 66), false);
    styleToolbarButton(scissorsButton, juce::Colour(62, 68, 84), false);
    styleToolbarButton(eraserButton, juce::Colour(62, 68, 84), false);
    styleToolbarButton(glueButton, juce::Colour(62, 68, 84), false);
    styleToolbarButton(loadButton, juce::Colour(62, 68, 84), false);
    styleToolbarButton(exportButton, juce::Colour(62, 68, 84), false);
    styleToolbarButton(projectButton, juce::Colour(62, 68, 84), false);
    styleToolbarButton(addTrackButton, juce::Colour(62, 68, 84), false);
    styleToolbarButton(settingsButton, juce::Colour(62, 68, 84), false);
    playButton.setTooltip("Play from start");
    stopButton.setTooltip("Stop and return playhead to start");
    scissorsButton.setTooltip("Scissors tool (X): split and trim clips");
    eraserButton.setTooltip("Eraser tool (E): delete clips");
    glueButton.setTooltip("Glue tool (G): merge selected clips");

    snapToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white.withAlpha(0.90f));
    snapToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(130, 200, 255));
    snapToggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(70, 76, 90));
    snapDivisionBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(43, 49, 61));
    snapDivisionBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.16f));
    snapDivisionBox.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.92f));
    snapDivisionBox.setColour(juce::ComboBox::arrowColourId, juce::Colours::white.withAlpha(0.70f));
    snapDivisionBox.addItem("1 Bar", 1);
    snapDivisionBox.addItem("1 Beat", 2);
    snapDivisionBox.addItem("1/8", 3);
    snapDivisionBox.addItem("1/16", 4);
    snapDivisionBox.addItem("1/32", 5);
    snapDivisionBox.addItem("1/8T", 6);
    snapDivisionBox.addItem("1/16T", 7);
    snapDivisionBox.setSelectedId(4, juce::dontSendNotification);

    scissorsButton.setClickingTogglesState(true);
    eraserButton.setClickingTogglesState(true);
    glueButton.setClickingTogglesState(true);
    scissorsButton.setToggleState(false, juce::dontSendNotification);
    eraserButton.setToggleState(false, juce::dontSendNotification);
    glueButton.setToggleState(false, juce::dontSendNotification);
    arrangementView.setEditTool(ArrangementView::EditTool::select);

    engine.addTrack("Track 1");

    tempoLabel.setText("Tempo", juce::dontSendNotification);
    timeSigLabel.setText("Time Sig", juce::dontSendNotification);
    tempoLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
    timeSigLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));

    tempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tempoSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 22);
    tempoSlider.setRange(40.0, 240.0, 0.1);
    tempoSlider.onValueChange = [this]() { applyTrackTimingFromControls(); };

    timeSigNumBox.addItem("2", 2);
    timeSigNumBox.addItem("3", 3);
    timeSigNumBox.addItem("4", 4);
    timeSigNumBox.addItem("5", 5);
    timeSigNumBox.addItem("6", 6);
    timeSigNumBox.addItem("7", 7);
    timeSigNumBox.addItem("9", 9);
    timeSigNumBox.addItem("12", 12);
    timeSigNumBox.onChange = [this]() { applyTrackTimingFromControls(); };

    timeSigDenBox.addItem("2", 2);
    timeSigDenBox.addItem("4", 4);
    timeSigDenBox.addItem("8", 8);
    timeSigDenBox.addItem("16", 16);
    timeSigDenBox.onChange = [this]() { applyTrackTimingFromControls(); };

    snapToggle.onClick = [this]() { applyTrackTimingFromControls(); };
    snapDivisionBox.onChange = [this]() { applyTrackTimingFromControls(); };

    // Track timing is edited inline in track headers now.
    tempoSlider.setVisible(false);
    timeSigNumBox.setVisible(false);
    timeSigDenBox.setVisible(false);
    tempoLabel.setVisible(false);
    timeSigLabel.setVisible(false);

    curveEditor.onCurveChanged([this](const WarpCurve& curve)
    {
        applyWarpToSelectedClip(curve, "Custom");
    });

    warpPanel.onPresetSelected([this](const WarpCurve& curve)
    {
        applyWarpToSelectedClip(curve, "Preset");
    });

    warpPanel.onCurveEdited([this](const WarpCurve& curve)
    {
        applyWarpToSelectedClip(curve, "Points");
    });
    warpPanel.onFitSettingsEdited([this](int fitLengthUnits, bool fitToSnapDivision)
    {
        applyFitSettingsToSelectedClip(fitLengthUnits, fitToSnapDivision);
    });
    beatSlicer.onSlicingChanged([this](const BeatSlicingSettings& slicing)
    {
        applySlicingToSelectedClip(slicing);
    });
    beatSlicer.onClose([this]()
    {
        beatSlicer.setVisible(false);
    });

    arrangementView.onClipSelected([this](int trackIndex, int clipIndex)
    {
        selectClip(trackIndex, clipIndex);
    });
    arrangementView.onTrackSelected([this](int trackIndex)
    {
        selectTrack(trackIndex);
    });
    arrangementView.onClipMoved([this](int sourceTrackIndex, int clipIndex, int targetTrackIndex, int64 newStartSample)
    {
        moveClip(sourceTrackIndex, clipIndex, targetTrackIndex, newStartSample);
    });
    arrangementView.onClipSplitRequested([this](int trackIndex, int clipIndex, int64 splitSample)
    {
        splitClip(trackIndex, clipIndex, splitSample);
    });
    arrangementView.onClipTrimRequested([this](int trackIndex, int clipIndex, int64 newStartSample, int64 newLengthSamples, float newSourceStartNorm, float newSourceEndNorm)
    {
        trimClip(trackIndex, clipIndex, newStartSample, newLengthSamples, newSourceStartNorm, newSourceEndNorm);
    });
    arrangementView.onClipDeleteRequested([this](int trackIndex, int clipIndex)
    {
        deleteClip(trackIndex, clipIndex);
    });
    arrangementView.onClipGlueRequested([this](const juce::Array<ArrangementView::ClipRef>& refs)
    {
        glueClips(refs);
    });
    arrangementView.onClipGainEdited([this](int trackIndex, int clipIndex, float gain)
    {
        editClipGain(trackIndex, clipIndex, gain);
    });
    arrangementView.onClipFadeEdited([this](int trackIndex, int clipIndex, float fadeInNorm, float fadeOutNorm)
    {
        editClipFade(trackIndex, clipIndex, fadeInNorm, fadeOutNorm);
    });
    arrangementView.onClipFitEdited([this](int trackIndex, int clipIndex, int fitLengthUnits, bool fitToSnapDivision)
    {
        editClipFit(trackIndex, clipIndex, fitLengthUnits, fitToSnapDivision);
    });
    arrangementView.onTrackNameEdited([this](int trackIndex, const juce::String& name)
    {
        editTrackName(trackIndex, name);
    });
    arrangementView.onTrackTempoEdited([this](int trackIndex, double tempoBpm)
    {
        editTrackTempo(trackIndex, tempoBpm);
    });
    arrangementView.onTrackTimeSigEdited([this](int trackIndex, int numerator, int denominator)
    {
        editTrackTimeSignature(trackIndex, numerator, denominator);
    });
    arrangementView.onTrackLoopMarkerEdited([this](int trackIndex, int markerIndex, int64 samplePosition, bool enabled)
    {
        editTrackLoopMarker(trackIndex, markerIndex, samplePosition, enabled);
    });
    arrangementView.onTrackZoomXEdited([this](int trackIndex, double zoomValue)
    {
        editTrackZoomX(trackIndex, zoomValue);
    });
    arrangementView.onTrackZoomYEdited([this](int trackIndex, double zoomValue)
    {
        editTrackZoomY(trackIndex, zoomValue);
    });
    arrangementView.onTrackVolumeEdited([this](int trackIndex, double volumeValue)
    {
        editTrackVolume(trackIndex, volumeValue);
    });
    arrangementView.onTrackPanEdited([this](int trackIndex, double panValue)
    {
        editTrackPan(trackIndex, panValue);
    });
    arrangementView.onTrackAutomationEdited([this](int trackIndex, bool isVolumeLane, const juce::Array<TrackAutomationPoint>& points)
    {
        editTrackAutomationPoints(trackIndex, isVolumeLane, points);
    });
    arrangementView.onTrackDeleteRequested([this](int trackIndex)
    {
        deleteTrack(trackIndex);
    });
    arrangementView.onUndoRequested([this]()
    {
        performUndo();
    });
    arrangementView.onCopyRequested([this]()
    {
        performCopy();
    });
    arrangementView.onPasteRequested([this]()
    {
        performPaste();
    });
    arrangementView.onCanDragPlayhead([this]()
    {
        return !engine.isPlaying();
    });
    arrangementView.onPlayheadDragged([this](int64 samplePosition)
    {
        if (engine.isPlaying())
            return;
        engine.setPlayheadSample(juce::jmax<int64>(0, samplePosition));
        arrangementView.setPlayheadSample(engine.getPlayheadSample());
    });
    arrangementView.onLayoutChanged([this]()
    {
        updateArrangementViewportBounds();
    });

    selectedTrackIndex = 0;
    selectedClipIndex = -1;
    arrangementView.setSelectedClip(selectedTrackIndex, selectedClipIndex);
    updateTrackControlsFromSelection();

    loadAppSettings();
    setAudioChannels(0, 2);
    if (pendingAudioDeviceState != nullptr)
    {
        deviceManager.initialise(0, 2, pendingAudioDeviceState.get(), true);
        pendingAudioDeviceState.reset();
    }
    startTimerHz(30);
    setWantsKeyboardFocus(true);
    grabKeyboardFocus();
}

MainComponent::~MainComponent()
{
    stopTimer();
    saveAppSettings();
    shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRateIn)
{
    sampleRate = sampleRateIn;
    engine.prepare(sampleRate, samplesPerBlockExpected);
    arrangementView.setTracks(&engine.getTracks(), sampleRate);
    updateArrangementViewportBounds();
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    engine.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    engine.release();
}

void MainComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient background(juce::Colour(30, 34, 42), 0.0f, 0.0f,
                                    juce::Colour(16, 18, 24), 0.0f, bounds.getBottom(), false);
    g.setGradientFill(background);
    g.fillRect(bounds);

    auto topBarBounds = getLocalBounds().removeFromTop(88).reduced(8, 6).toFloat();
    juce::ColourGradient topBar(juce::Colour(74, 79, 91), 0.0f, topBarBounds.getY(),
                                juce::Colour(42, 46, 56), 0.0f, topBarBounds.getBottom(), false);
    g.setGradientFill(topBar);
    g.fillRoundedRectangle(topBarBounds, 8.0f);
    g.setColour(juce::Colour(255, 255, 255).withAlpha(0.16f));
    g.drawRoundedRectangle(topBarBounds, 8.0f, 1.0f);
    g.setColour(juce::Colour(0, 0, 0).withAlpha(0.24f));
    g.drawLine(topBarBounds.getX(), topBarBounds.getBottom(), topBarBounds.getRight(), topBarBounds.getBottom(), 1.0f);

    g.setColour(juce::Colour(255, 255, 255).withAlpha(0.08f));
    g.fillRect(0, 88, getWidth(), 1);
}

void MainComponent::resized()
{
    const int padding = 10;
    const int topBarH = 88;
    const int sectionGap = 16;
    const int buttonGap = 6;

    auto area = getLocalBounds();
    auto top = area.removeFromTop(topBarH).reduced(padding + 8, 10);
    auto labelsRow = top.removeFromTop(16);
    top.removeFromTop(4);
    auto controlsRow = top.removeFromTop(30);

    const int transportW = 70 + buttonGap + 70 + buttonGap + 86 + buttonGap + 74 + buttonGap + 64 + buttonGap + 72 + buttonGap + 98;
    const int mediaW = 95 + buttonGap + 108;
    const int projectW = 98 + buttonGap + 98;
    const int trackW = 92;

    auto transportLabelArea = labelsRow.removeFromLeft(transportW);
    transportSectionLabel.setBounds(transportLabelArea);
    labelsRow.removeFromLeft(sectionGap);
    auto mediaLabelArea = labelsRow.removeFromLeft(mediaW);
    mediaSectionLabel.setBounds(mediaLabelArea);
    labelsRow.removeFromLeft(sectionGap);
    trackSectionLabel.setBounds(labelsRow.removeFromLeft(trackW));
    labelsRow.removeFromLeft(sectionGap);
    auto projectLabelArea = labelsRow.removeFromLeft(projectW);
    projectSectionLabel.setBounds(projectLabelArea);

    auto transportControls = controlsRow.removeFromLeft(transportW);
    playButton.setBounds(transportControls.removeFromLeft(70));
    transportControls.removeFromLeft(buttonGap);
    stopButton.setBounds(transportControls.removeFromLeft(70));
    transportControls.removeFromLeft(buttonGap);
    scissorsButton.setBounds(transportControls.removeFromLeft(86));
    transportControls.removeFromLeft(buttonGap);
    eraserButton.setBounds(transportControls.removeFromLeft(74));
    transportControls.removeFromLeft(buttonGap);
    glueButton.setBounds(transportControls.removeFromLeft(64));
    transportControls.removeFromLeft(buttonGap);
    snapToggle.setBounds(transportControls.removeFromLeft(72));
    transportControls.removeFromLeft(buttonGap);
    snapDivisionBox.setBounds(transportControls.removeFromLeft(98));

    controlsRow.removeFromLeft(sectionGap);
    auto mediaControls = controlsRow.removeFromLeft(mediaW);
    loadButton.setBounds(mediaControls.removeFromLeft(95));
    mediaControls.removeFromLeft(buttonGap);
    exportButton.setBounds(mediaControls.removeFromLeft(108));

    controlsRow.removeFromLeft(sectionGap);
    auto trackControls = controlsRow.removeFromLeft(trackW);
    addTrackButton.setBounds(trackControls.removeFromLeft(92));

    controlsRow.removeFromLeft(sectionGap);
    auto projectControls = controlsRow.removeFromLeft(projectW);
    projectButton.setBounds(projectControls.removeFromLeft(98));
    projectControls.removeFromLeft(buttonGap);
    settingsButton.setBounds(projectControls.removeFromLeft(98));

    area.removeFromTop(6);
    auto bottom = area.removeFromBottom(220).reduced(padding);
    auto panelArea = bottom.removeFromRight(300);
    warpPanel.setBounds(panelArea);
    curveEditor.setBounds(bottom);
    arrangementViewport.setBounds(area.reduced(padding));
    updateArrangementViewportBounds();
    beatSlicer.setBounds(getLocalBounds().reduced(70, 70));
}

void MainComponent::updateArrangementViewportBounds(bool preserveScroll)
{
    const auto viewportBounds = arrangementViewport.getLocalBounds();
    if (viewportBounds.isEmpty())
        return;

    const int previousScrollY = arrangementViewport.getViewPositionY();
    const int contentWidth = viewportBounds.getWidth();
    const int contentHeight = juce::jmax(viewportBounds.getHeight(), arrangementView.getRequiredContentHeight());
    arrangementView.setBounds(0, 0, contentWidth, contentHeight);
    arrangementView.refreshTrackControls();

    if (preserveScroll)
    {
        const int maxScrollY = juce::jmax(0, contentHeight - viewportBounds.getHeight());
        arrangementViewport.setViewPosition(arrangementViewport.getViewPositionX(),
                                            juce::jlimit(0, maxScrollY, previousScrollY));
    }
    else
    {
        arrangementViewport.setViewPosition(arrangementViewport.getViewPositionX(), 0);
    }
}

void MainComponent::styleToolbarButton(juce::TextButton& button, juce::Colour baseColour, bool isPrimary)
{
    const juce::Colour normal = baseColour;
    const juce::Colour pressed = baseColour.brighter(0.18f);
    button.setColour(juce::TextButton::buttonColourId, normal);
    button.setColour(juce::TextButton::buttonOnColourId, pressed);
    button.setColour(juce::TextButton::textColourOffId, juce::Colour(238, 242, 247).withAlpha(0.96f));
    button.setColour(juce::TextButton::textColourOnId, juce::Colours::white.withAlpha(0.98f));
    button.setConnectedEdges(0);
    if (isPrimary)
        button.setTriggeredOnMouseDown(false);
}

void MainComponent::setActiveEditTool(ArrangementView::EditTool tool)
{
    arrangementView.setEditTool(tool);
    scissorsButton.setToggleState(tool == ArrangementView::EditTool::scissors, juce::dontSendNotification);
    eraserButton.setToggleState(tool == ArrangementView::EditTool::eraser, juce::dontSendNotification);
    glueButton.setToggleState(tool == ArrangementView::EditTool::glue, juce::dontSendNotification);
    styleToolbarButton(scissorsButton,
                       tool == ArrangementView::EditTool::scissors ? juce::Colour(96, 88, 56) : juce::Colour(62, 68, 84),
                       false);
    styleToolbarButton(eraserButton,
                       tool == ArrangementView::EditTool::eraser ? juce::Colour(98, 62, 62) : juce::Colour(62, 68, 84),
                       false);
    styleToolbarButton(glueButton,
                       tool == ArrangementView::EditTool::glue ? juce::Colour(66, 96, 76) : juce::Colour(62, 68, 84),
                       false);
}

void MainComponent::buttonClicked(juce::Button* button)
{
    if (button == &playButton)
    {
        engine.setPlayheadSample(0);
        arrangementView.setPlayheadSample(0);
        engine.setPlaying(true);
    }
    else if (button == &stopButton)
    {
        engine.setPlaying(false);
        engine.setPlayheadSample(0);
    }
    else if (button == &scissorsButton)
    {
        if (scissorsButton.getToggleState())
            setActiveEditTool(ArrangementView::EditTool::scissors);
        else
            setActiveEditTool(ArrangementView::EditTool::select);
    }
    else if (button == &eraserButton)
    {
        if (eraserButton.getToggleState())
            setActiveEditTool(ArrangementView::EditTool::eraser);
        else
            setActiveEditTool(ArrangementView::EditTool::select);
    }
    else if (button == &glueButton)
    {
        if (glueButton.getToggleState())
            setActiveEditTool(ArrangementView::EditTool::glue);
        else
            setActiveEditTool(ArrangementView::EditTool::select);
    }
    else if (button == &loadButton)
    {
        showLoadDialog();
    }
    else if (button == &exportButton)
    {
        showExportDialog();
    }
    else if (button == &projectButton)
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Save Project");
        menu.addItem(2, "Load Project");
        juce::Component::SafePointer<MainComponent> safeThis(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&projectButton),
                           [safeThis](int result)
                           {
                               if (safeThis == nullptr || result == 0)
                                   return;
                               if (result == 1)
                                   safeThis->showSaveProjectDialog();
                               else if (result == 2)
                                   safeThis->showLoadProjectDialog();
                           });
    }
    else if (button == &settingsButton)
    {
        showAudioSettingsDialog();
    }
    else if (button == &addTrackButton)
    {
        pushUndoState();
        int newTrackIndex = -1;
        {
            juce::ScopedLock lock(engine.getTrackLock());
            const int nextIndex = engine.getTracks().size() + 1;
            engine.addTrack("Track " + juce::String(nextIndex));
            selectedTrackIndex = engine.getTracks().size() - 1;
            selectedClipIndex = -1;
            newTrackIndex = selectedTrackIndex;
        }

        arrangementView.setSelectedClip(newTrackIndex, -1);
        arrangementView.refreshTrackControls();
        updateArrangementViewportBounds();
        updateTrackControlsFromSelection();
    }
}

void MainComponent::timerCallback()
{
    arrangementView.setPlayheadSample(engine.getPlayheadSample());
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    const auto mods = key.getModifiers();
    const int kc = key.getKeyCode();

    if (kc == juce::KeyPress::spaceKey)
    {
        engine.setPlaying(!engine.isPlaying());
        return true;
    }

    if (mods.isCommandDown() && (kc == 'z' || kc == 'Z'))
    {
        performUndo();
        return true;
    }

    if (mods.isCommandDown() && (kc == 'c' || kc == 'C'))
    {
        performCopy();
        return true;
    }

    if (mods.isCommandDown() && (kc == 'v' || kc == 'V'))
    {
        performPaste();
        return true;
    }

    if (kc == 'x' || kc == 'X')
    {
        const bool next = arrangementView.getEditTool() != ArrangementView::EditTool::scissors;
        setActiveEditTool(next ? ArrangementView::EditTool::scissors : ArrangementView::EditTool::select);
        return true;
    }

    if (kc == 'e' || kc == 'E')
    {
        const bool next = arrangementView.getEditTool() != ArrangementView::EditTool::eraser;
        setActiveEditTool(next ? ArrangementView::EditTool::eraser : ArrangementView::EditTool::select);
        return true;
    }

    if (kc == 'g' || kc == 'G')
    {
        const bool next = arrangementView.getEditTool() != ArrangementView::EditTool::glue;
        setActiveEditTool(next ? ArrangementView::EditTool::glue : ArrangementView::EditTool::select);
        return true;
    }

    if (key.getKeyCode() == juce::KeyPress::backspaceKey || key.getKeyCode() == juce::KeyPress::deleteKey)
    {
        if (selectedTrackIndex >= 0)
        {
            deleteTrack(selectedTrackIndex);
            return true;
        }
    }

    if (key == juce::KeyPress('s', 0, 0) || key == juce::KeyPress('S', 0, 0))
    {
        openBeatSlicerForSelection();
        return true;
    }
    return false;
}

void MainComponent::showLoadDialog()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load WAV",
        juce::File{},
        "*.wav");

    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    juce::Component::SafePointer<MainComponent> safeThis(this);
    fileChooser->launchAsync(flags, [safeThis](const juce::FileChooser& chooser)
    {
        if (safeThis == nullptr)
            return;

        const auto file = chooser.getResult();
        if (file.existsAsFile())
            safeThis->addSampleToTrack(file);
    });
}

void MainComponent::showSaveProjectDialog()
{
    projectFileChooser = std::make_unique<juce::FileChooser>(
        "Save Project",
        juce::File{},
        "*.drdproj");

    auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
    juce::Component::SafePointer<MainComponent> safeThis(this);
    projectFileChooser->launchAsync(flags, [safeThis](const juce::FileChooser& chooser)
    {
        if (safeThis == nullptr)
            return;

        auto file = chooser.getResult();
        if (file == juce::File{})
            return;
        if (!file.hasFileExtension(".drdproj"))
            file = file.withFileExtension(".drdproj");

        safeThis->saveProjectToFile(file);
    });
}

void MainComponent::showLoadProjectDialog()
{
    projectFileChooser = std::make_unique<juce::FileChooser>(
        "Load Project",
        juce::File{},
        "*.drdproj");

    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    juce::Component::SafePointer<MainComponent> safeThis(this);
    projectFileChooser->launchAsync(flags, [safeThis](const juce::FileChooser& chooser)
    {
        if (safeThis == nullptr)
            return;

        const auto file = chooser.getResult();
        if (file.existsAsFile())
        {
            safeThis->pushUndoState();
            safeThis->loadProjectFromFile(file);
        }
    });
}

void MainComponent::showAudioSettingsDialog()
{
    auto content = std::make_unique<AudioSettingsPane>(deviceManager,
                                                       exportBitDepth,
                                                       [this](int bitDepth)
                                                       {
                                                           exportBitDepth = juce::jlimit(16, 32, bitDepth);
                                                           saveAppSettings();
                                                       });

    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Audio Settings";
    options.dialogBackgroundColour = juce::Colour(30, 34, 42);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = true;
    options.componentToCentreAround = this;
    options.content.setOwned(content.release());
    options.content->setSize(600, 460);
    options.launchAsync();
}

juce::File MainComponent::getAppSettingsFile() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("MMMSample");
    if (!dir.exists())
        dir.createDirectory();
    return dir.getChildFile("settings.xml");
}

void MainComponent::loadAppSettings()
{
    const auto file = getAppSettingsFile();
    if (!file.existsAsFile())
        return;

    std::unique_ptr<juce::XmlElement> root(juce::XmlDocument::parse(file));
    if (root == nullptr || !root->hasTagName("MMMSampleSettings"))
        return;

    if (root->hasAttribute("exportBitDepth"))
        exportBitDepth = juce::jlimit(16, 32, root->getIntAttribute("exportBitDepth", exportBitDepth));

    if (auto* audioState = root->getChildByName("audioDeviceState"))
        pendingAudioDeviceState = std::make_unique<juce::XmlElement>(*audioState);
}

void MainComponent::saveAppSettings() const
{
    juce::XmlElement root("MMMSampleSettings");
    root.setAttribute("version", 1);
    root.setAttribute("exportBitDepth", exportBitDepth);

    if (auto state = deviceManager.createStateXml())
    {
        auto* audioState = new juce::XmlElement("audioDeviceState");
        audioState->addChildElement(state.release());
        root.addChildElement(audioState);
    }

    auto file = getAppSettingsFile();
    file.replaceWithText(root.toString(), false, false, "\n");
}

void MainComponent::addSampleToTrack(const juce::File& file)
{
    auto sample = std::make_shared<Sample>();
    if (!sample->loadFromFile(file))
        return;
    pushUndoState();

    int newTrackIndex = 0;
    int newClipIndex = -1;
    WarpCurve clipCurve = WarpCurve::linear();
    int64 clipStartSample = 0;
    int64 clipLengthSamples = (int64)(sampleRate * 2.0);

    {
        juce::ScopedLock lock(engine.getTrackLock());
        if (engine.getTracks().isEmpty())
            engine.addTrack("Track 1");

        newTrackIndex = selectedTrackIndex;
        if (newTrackIndex < 0 || newTrackIndex >= engine.getTracks().size())
            newTrackIndex = 0;

        auto& track = engine.getTracks().getReference(newTrackIndex);
        clipStartSample = track.snapSampleToGrid(engine.getPlayheadSample(), sampleRate);
        clipLengthSamples = track.getBarLengthSamples(sampleRate);
        if (clipLengthSamples <= 0)
            clipLengthSamples = (int64)(sampleRate * 2.0);

        TrackClip clip;
        clip.name = file.getFileNameWithoutExtension();
        clip.sourceFilePath = file.getFullPathName();
        clip.sample = sample;
        clip.startSample = clipStartSample;
        clip.lengthSamples = clipLengthSamples;
        clip.sourceStartNorm = 0.0f;
        clip.sourceEndNorm = 1.0f;
        clip.fitLengthUnits = 1;
        clip.fitToSnapDivision = false;
        clip.gain = 0.9f;
        clip.warpCurve = clipCurve;

        track.addClip(clip);
        selectedTrackIndex = newTrackIndex;
        selectedClipIndex = track.getClips().size() - 1;
        newTrackIndex = selectedTrackIndex;
        newClipIndex = selectedClipIndex;
    }

    arrangementView.setSelectedClip(newTrackIndex, newClipIndex);
    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds();
    curveEditor.setCurve(clipCurve);
    warpPanel.setCurve(clipCurve);
    warpPanel.setFitSettings(1, false);
}

juce::var MainComponent::createProjectStateVar()
{
    juce::var rootObjVar(new juce::DynamicObject());
    auto* rootObj = rootObjVar.getDynamicObject();
    if (rootObj == nullptr)
        return {};

    rootObj->setProperty("version", 1);
    rootObj->setProperty("sampleRate", sampleRate);
    rootObj->setProperty("exportBitDepth", exportBitDepth);
    rootObj->setProperty("selectedTrackIndex", selectedTrackIndex);
    rootObj->setProperty("selectedClipIndex", selectedClipIndex);
    rootObj->setProperty("playheadSample", engine.getPlayheadSample());

    juce::Array<juce::var> tracksArray;
    {
        juce::ScopedLock lock(engine.getTrackLock());
        const auto& tracks = engine.getTracks();
        for (const auto& track : tracks)
        {
            auto* trackObj = new juce::DynamicObject();
            trackObj->setProperty("name", track.getName());
            trackObj->setProperty("tempoBpm", track.getTempoBpm());
            trackObj->setProperty("timeSigNum", track.getTimeSigNumerator());
            trackObj->setProperty("timeSigDen", track.getTimeSigDenominator());
            trackObj->setProperty("snapEnabled", track.isSnapEnabled());
            trackObj->setProperty("snapDivision", snapDivisionToId(track.getSnapDivision()));
            trackObj->setProperty("zoomX", track.getZoomX());
            trackObj->setProperty("zoomY", track.getZoomY());
            trackObj->setProperty("volume", track.getVolume());
            trackObj->setProperty("pan", track.getPan());
            juce::Array<juce::var> volAuto;
            for (const auto& p : track.getVolumeAutomation())
            {
                auto* pObj = new juce::DynamicObject();
                pObj->setProperty("sample", (double)p.samplePosition);
                pObj->setProperty("value", p.value);
                volAuto.add(juce::var(pObj));
            }
            juce::Array<juce::var> panAuto;
            for (const auto& p : track.getPanAutomation())
            {
                auto* pObj = new juce::DynamicObject();
                pObj->setProperty("sample", (double)p.samplePosition);
                pObj->setProperty("value", p.value);
                panAuto.add(juce::var(pObj));
            }
            trackObj->setProperty("volumeAutomation", juce::var(volAuto));
            trackObj->setProperty("panAutomation", juce::var(panAuto));
            trackObj->setProperty("loopMarker1Enabled", track.hasLoopMarker(0));
            trackObj->setProperty("loopMarker2Enabled", track.hasLoopMarker(1));
            trackObj->setProperty("loopMarker1Sample", (double)track.getLoopMarker(0));
            trackObj->setProperty("loopMarker2Sample", (double)track.getLoopMarker(1));

            juce::Array<juce::var> clipsArray;
            for (const auto& clip : track.getClips())
            {
                auto* clipObj = new juce::DynamicObject();
                clipObj->setProperty("name", clip.name);
                clipObj->setProperty("sourceFilePath", clip.sourceFilePath);
                clipObj->setProperty("startSample", (double)clip.startSample);
                clipObj->setProperty("lengthSamples", (double)clip.lengthSamples);
                clipObj->setProperty("sourceStartNorm", clip.sourceStartNorm);
                clipObj->setProperty("sourceEndNorm", clip.sourceEndNorm);
                clipObj->setProperty("fitLengthUnits", clip.fitLengthUnits);
                clipObj->setProperty("fitToSnapDivision", clip.fitToSnapDivision);
                clipObj->setProperty("gain", clip.gain);
                clipObj->setProperty("fadeInNorm", clip.fadeInNorm);
                clipObj->setProperty("fadeOutNorm", clip.fadeOutNorm);
                clipObj->setProperty("muted", clip.muted);
                clipObj->setProperty("warpPoints", warpCurveToVar(clip.warpCurve));
                clipObj->setProperty("slicingEnabled", clip.slicing.enabled);
                clipObj->setProperty("slicingMode", sliceModeToString(clip.slicing.mode));
                juce::Array<juce::var> sliceArray;
                for (const auto& s : clip.slicing.slices)
                {
                    auto* sObj = new juce::DynamicObject();
                    sObj->setProperty("start", s.startNorm);
                    sObj->setProperty("end", s.endNorm);
                    sObj->setProperty("playProb", s.playProbability);
                    sObj->setProperty("ratchetRepeats", s.ratchetRepeats);
                    sObj->setProperty("repeatProb", s.repeatProbability);
                    sliceArray.add(juce::var(sObj));
                }
                clipObj->setProperty("slices", juce::var(sliceArray));
                clipsArray.add(juce::var(clipObj));
            }

            trackObj->setProperty("clips", juce::var(clipsArray));
            tracksArray.add(juce::var(trackObj));
        }
    }

    rootObj->setProperty("tracks", juce::var(tracksArray));
    return rootObjVar;
}

bool MainComponent::saveProjectToFile(const juce::File& file)
{
    const auto rootObjVar = createProjectStateVar();
    if (rootObjVar.isVoid())
        return false;
    const auto json = juce::JSON::toString(rootObjVar, true);
    return file.replaceWithText(json);
}

bool MainComponent::loadProjectFromVar(const juce::var& parsed, bool preserveScroll)
{
    auto* rootObj = parsed.getDynamicObject();
    if (rootObj == nullptr)
        return false;

    const auto tracksVar = rootObj->getProperty("tracks");
    if (!tracksVar.isArray())
        return false;

    if (rootObj->hasProperty("exportBitDepth"))
        exportBitDepth = juce::jlimit(16, 32, (int)rootObj->getProperty("exportBitDepth"));

    juce::Array<Track> loadedTracks;
    auto* tracksArray = tracksVar.getArray();
    for (const auto& trackVar : *tracksArray)
    {
        auto* trackObj = trackVar.getDynamicObject();
        if (trackObj == nullptr)
            continue;

        Track track(trackObj->getProperty("name").toString());
        track.setTempoBpm((double)trackObj->getProperty("tempoBpm"));
        track.setTimeSignature((int)trackObj->getProperty("timeSigNum"), (int)trackObj->getProperty("timeSigDen"));
        track.setSnapEnabled((bool)trackObj->getProperty("snapEnabled"));
        if (trackObj->hasProperty("snapDivision"))
            track.setSnapDivision(idToSnapDivision((int)trackObj->getProperty("snapDivision")));
        if (trackObj->hasProperty("zoomX"))
            track.setZoomX((double)trackObj->getProperty("zoomX"));
        if (trackObj->hasProperty("zoomY"))
            track.setZoomY((double)trackObj->getProperty("zoomY"));
        if (trackObj->hasProperty("volume"))
            track.setVolume((double)trackObj->getProperty("volume"));
        if (trackObj->hasProperty("pan"))
            track.setPan((double)trackObj->getProperty("pan"));
        if (trackObj->hasProperty("volumeAutomation") && trackObj->getProperty("volumeAutomation").isArray())
        {
            juce::Array<TrackAutomationPoint> points;
            auto* arr = trackObj->getProperty("volumeAutomation").getArray();
            for (const auto& item : *arr)
            {
                auto* pObj = item.getDynamicObject();
                if (pObj == nullptr)
                    continue;
                TrackAutomationPoint p;
                p.samplePosition = (int64)(double)pObj->getProperty("sample");
                p.value = (double)pObj->getProperty("value");
                points.add(p);
            }
            track.setVolumeAutomation(points);
        }
        if (trackObj->hasProperty("panAutomation") && trackObj->getProperty("panAutomation").isArray())
        {
            juce::Array<TrackAutomationPoint> points;
            auto* arr = trackObj->getProperty("panAutomation").getArray();
            for (const auto& item : *arr)
            {
                auto* pObj = item.getDynamicObject();
                if (pObj == nullptr)
                    continue;
                TrackAutomationPoint p;
                p.samplePosition = (int64)(double)pObj->getProperty("sample");
                p.value = (double)pObj->getProperty("value");
                points.add(p);
            }
            track.setPanAutomation(points);
        }
        if ((bool)trackObj->getProperty("loopMarker1Enabled"))
            track.setLoopMarker(0, (int64)(double)trackObj->getProperty("loopMarker1Sample"));
        if ((bool)trackObj->getProperty("loopMarker2Enabled"))
            track.setLoopMarker(1, (int64)(double)trackObj->getProperty("loopMarker2Sample"));

        const auto clipsVar = trackObj->getProperty("clips");
        if (clipsVar.isArray())
        {
            auto* clipsArray = clipsVar.getArray();
            for (const auto& clipVar : *clipsArray)
            {
                auto* clipObj = clipVar.getDynamicObject();
                if (clipObj == nullptr)
                    continue;

                const juce::String sourcePath = clipObj->getProperty("sourceFilePath").toString();
                const juce::File sampleFile(sourcePath);
                if (!sampleFile.existsAsFile())
                    continue;

                auto sample = std::make_shared<Sample>();
                if (!sample->loadFromFile(sampleFile))
                    continue;

                TrackClip clip;
                clip.name = clipObj->getProperty("name").toString();
                clip.sourceFilePath = sourcePath;
                clip.sample = sample;
                clip.startSample = (int64)(double)clipObj->getProperty("startSample");
                clip.lengthSamples = (int64)(double)clipObj->getProperty("lengthSamples");
                if (clipObj->hasProperty("sourceStartNorm"))
                    clip.sourceStartNorm = (float)(double)clipObj->getProperty("sourceStartNorm");
                if (clipObj->hasProperty("sourceEndNorm"))
                    clip.sourceEndNorm = (float)(double)clipObj->getProperty("sourceEndNorm");
                clip.sourceStartNorm = juce::jlimit(0.0f, 1.0f, clip.sourceStartNorm);
                clip.sourceEndNorm = juce::jlimit(clip.sourceStartNorm, 1.0f, clip.sourceEndNorm);
                if (clipObj->hasProperty("fitLengthUnits"))
                    clip.fitLengthUnits = juce::jmax(1, (int)clipObj->getProperty("fitLengthUnits"));
                if (clipObj->hasProperty("fitToSnapDivision"))
                    clip.fitToSnapDivision = (bool)clipObj->getProperty("fitToSnapDivision");
                clip.gain = (float)(double)clipObj->getProperty("gain");
                if (clipObj->hasProperty("fadeInNorm"))
                    clip.fadeInNorm = (float)(double)clipObj->getProperty("fadeInNorm");
                if (clipObj->hasProperty("fadeOutNorm"))
                    clip.fadeOutNorm = (float)(double)clipObj->getProperty("fadeOutNorm");
                clip.muted = (bool)clipObj->getProperty("muted");
                clip.warpCurve = varToWarpCurve(clipObj->getProperty("warpPoints"));
                clip.slicing.enabled = (bool)clipObj->getProperty("slicingEnabled");
                clip.slicing.mode = stringToSliceMode(clipObj->getProperty("slicingMode").toString());
                const auto slicesVar = clipObj->getProperty("slices");
                if (slicesVar.isArray())
                {
                    auto* sArr = slicesVar.getArray();
                    for (const auto& item : *sArr)
                    {
                        auto* sObj = item.getDynamicObject();
                        if (sObj == nullptr)
                            continue;
                        BeatSlice s;
                        s.startNorm = (double)sObj->getProperty("start");
                        s.endNorm = (double)sObj->getProperty("end");
                        s.playProbability = (double)sObj->getProperty("playProb");
                        s.ratchetRepeats = (int)sObj->getProperty("ratchetRepeats");
                        s.repeatProbability = (double)sObj->getProperty("repeatProb");
                        clip.slicing.slices.add(s);
                    }
                }
                track.addClip(clip);
            }
        }

        loadedTracks.add(track);
    }

    {
        juce::ScopedLock lock(engine.getTrackLock());
        engine.getTracks().clear();
        for (const auto& t : loadedTracks)
            engine.getTracks().add(t);

        if (engine.getTracks().isEmpty())
            engine.getTracks().add(Track("Track 1"));
    }

    int trackCountAfterLoad = 0;
    {
        juce::ScopedLock lock(engine.getTrackLock());
        trackCountAfterLoad = engine.getTracks().size();
    }

    selectedTrackIndex = juce::jlimit(0, juce::jmax(0, trackCountAfterLoad - 1), (int)rootObj->getProperty("selectedTrackIndex"));
    selectedClipIndex = (int)rootObj->getProperty("selectedClipIndex");
    engine.setPlayheadSample((int64)(double)rootObj->getProperty("playheadSample"));

    const int previousScrollY = arrangementViewport.getViewPositionY();
    arrangementView.setTracks(&engine.getTracks(), sampleRate);
    arrangementView.setSelectedClip(selectedTrackIndex, selectedClipIndex);
    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds(preserveScroll);
    if (preserveScroll)
    {
        const int maxScrollY = juce::jmax(0, arrangementView.getHeight() - arrangementViewport.getHeight());
        arrangementViewport.setViewPosition(arrangementViewport.getViewPositionX(),
                                            juce::jlimit(0, maxScrollY, previousScrollY));
    }

    WarpCurve selectionCurve = WarpCurve::linear();
    {
        juce::ScopedLock lock(engine.getTrackLock());
        if (selectedTrackIndex >= 0 && selectedTrackIndex < engine.getTracks().size())
        {
            const auto& clips = engine.getTracks().getReference(selectedTrackIndex).getClips();
            if (selectedClipIndex >= 0 && selectedClipIndex < clips.size())
                selectionCurve = clips.getReference(selectedClipIndex).warpCurve;
            else
                selectedClipIndex = -1;
        }
    }

    curveEditor.setCurve(selectionCurve);
    warpPanel.setCurve(selectionCurve);
    if (selectedTrackIndex >= 0 && selectedTrackIndex < engine.getTracks().size() && selectedClipIndex >= 0)
    {
        juce::ScopedLock lock(engine.getTrackLock());
        const auto& clips = engine.getTracks().getReference(selectedTrackIndex).getClips();
        if (selectedClipIndex >= 0 && selectedClipIndex < clips.size())
            warpPanel.setFitSettings(clips.getReference(selectedClipIndex).fitLengthUnits,
                                     clips.getReference(selectedClipIndex).fitToSnapDivision);
    }
    updateTrackControlsFromSelection();
    return true;
}

bool MainComponent::loadProjectFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    const juce::String text = file.loadFileAsString();
    const juce::var parsed = juce::JSON::parse(text);
    return loadProjectFromVar(parsed, true);
}

void MainComponent::pushUndoState()
{
    if (isApplyingUndo)
        return;

    const auto stateVar = createProjectStateVar();
    if (stateVar.isVoid())
        return;

    const juce::String state = juce::JSON::toString(stateVar, false);
    if (!undoStack.isEmpty() && undoStack.getLast() == state)
        return;

    undoStack.add(state);
    while (undoStack.size() > 20)
        undoStack.remove(0);
}

void MainComponent::performUndo()
{
    if (undoStack.isEmpty())
        return;

    const juce::String state = undoStack.getLast();
    undoStack.removeLast();
    const juce::var parsed = juce::JSON::parse(state);

    const juce::ScopedValueSetter<bool> undoGuard(isApplyingUndo, true);
    loadProjectFromVar(parsed, true);
}

void MainComponent::performCopy()
{
    copiedTracks.clear();
    copiedClips.clear();
    copiedMarkers.clear();
    clipboardType = ClipboardType::none;

    const auto selectionType = arrangementView.getSelectionType();
    juce::ScopedLock lock(engine.getTrackLock());
    const auto& tracks = engine.getTracks();

    if (selectionType == ArrangementView::SelectionType::track)
    {
        for (const auto trackIndex : arrangementView.getSelectedTracks())
        {
            if (trackIndex >= 0 && trackIndex < tracks.size())
                copiedTracks.add(tracks.getReference(trackIndex));
        }
        if (!copiedTracks.isEmpty())
            clipboardType = ClipboardType::track;
    }
    else if (selectionType == ArrangementView::SelectionType::clip)
    {
        for (const auto& ref : arrangementView.getSelectedClips())
        {
            if (ref.trackIndex < 0 || ref.trackIndex >= tracks.size())
                continue;
            const auto& clips = tracks.getReference(ref.trackIndex).getClips();
            if (ref.clipIndex < 0 || ref.clipIndex >= clips.size())
                continue;

            CopiedClip cc;
            cc.sourceTrackIndex = ref.trackIndex;
            cc.startSample = clips.getReference(ref.clipIndex).startSample;
            cc.clip = clips.getReference(ref.clipIndex);
            copiedClips.add(cc);
        }
        if (!copiedClips.isEmpty())
            clipboardType = ClipboardType::clip;
    }
    else if (selectionType == ArrangementView::SelectionType::marker)
    {
        for (const auto& ref : arrangementView.getSelectedMarkers())
        {
            if (ref.trackIndex < 0 || ref.trackIndex >= tracks.size())
                continue;
            const auto& track = tracks.getReference(ref.trackIndex);
            if (!track.hasLoopMarker(ref.markerIndex))
                continue;

            CopiedMarker cm;
            cm.sourceTrackIndex = ref.trackIndex;
            cm.markerIndex = ref.markerIndex;
            cm.samplePosition = track.getLoopMarker(ref.markerIndex);
            copiedMarkers.add(cm);
        }
        if (!copiedMarkers.isEmpty())
            clipboardType = ClipboardType::marker;
    }

    // Fallback: if a single clip is selected in component state, allow copy even if view selection type isn't clip.
    if (clipboardType == ClipboardType::none)
    {
        if (selectedTrackIndex >= 0 && selectedTrackIndex < tracks.size())
        {
            const auto& clips = tracks.getReference(selectedTrackIndex).getClips();
            if (selectedClipIndex >= 0 && selectedClipIndex < clips.size())
            {
                CopiedClip cc;
                cc.sourceTrackIndex = selectedTrackIndex;
                cc.startSample = clips.getReference(selectedClipIndex).startSample;
                cc.clip = clips.getReference(selectedClipIndex);
                copiedClips.add(cc);
                clipboardType = ClipboardType::clip;
            }
        }
    }
}

void MainComponent::performPaste()
{
    if (clipboardType == ClipboardType::none)
        return;

    pushUndoState();

    int newSelectedTrack = selectedTrackIndex;
    int newSelectedClip = -1;
    const int64 playhead = engine.getPlayheadSample();

    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();

        if (clipboardType == ClipboardType::track)
        {
            for (int i = 0; i < copiedTracks.size(); ++i)
            {
                auto t = copiedTracks.getReference(i);
                t.setName(t.getName() + " Copy");
                tracks.add(t);
            }
            if (!tracks.isEmpty())
                newSelectedTrack = tracks.size() - 1;
        }
        else if (clipboardType == ClipboardType::clip)
        {
            if (tracks.isEmpty() || copiedClips.isEmpty())
                return;

            int64 minStart = std::numeric_limits<int64>::max();
            for (const auto& c : copiedClips)
                minStart = juce::jmin(minStart, c.startSample);
            if (minStart == std::numeric_limits<int64>::max())
                minStart = 0;

            for (int i = 0; i < copiedClips.size(); ++i)
            {
                auto cc = copiedClips.getReference(i);
                int targetTrack = (selectedTrackIndex >= 0 && selectedTrackIndex < tracks.size())
                                      ? selectedTrackIndex
                                      : cc.sourceTrackIndex;
                if (targetTrack < 0 || targetTrack >= tracks.size())
                    targetTrack = 0;

                auto clip = cc.clip;
                const int64 offset = cc.startSample - minStart;
                const int64 unsnapped = juce::jmax<int64>(0, playhead + offset);
                clip.startSample = tracks.getReference(targetTrack).snapSampleToGrid(unsnapped, sampleRate);
                tracks.getReference(targetTrack).addClip(clip);

                newSelectedTrack = targetTrack;
                newSelectedClip = tracks.getReference(targetTrack).getClips().size() - 1;
            }
        }
        else if (clipboardType == ClipboardType::marker)
        {
            if (tracks.isEmpty() || copiedMarkers.isEmpty())
                return;

            int64 minPos = std::numeric_limits<int64>::max();
            for (const auto& m : copiedMarkers)
                minPos = juce::jmin(minPos, m.samplePosition);
            if (minPos == std::numeric_limits<int64>::max())
                minPos = 0;

            for (const auto& cm : copiedMarkers)
            {
                int targetTrack = (selectedTrackIndex >= 0 && selectedTrackIndex < tracks.size())
                                      ? selectedTrackIndex
                                      : cm.sourceTrackIndex;
                if (targetTrack < 0 || targetTrack >= tracks.size())
                    targetTrack = 0;
                const int64 offset = cm.samplePosition - minPos;
                tracks.getReference(targetTrack).setLoopMarker(cm.markerIndex, juce::jmax<int64>(0, playhead + offset));
                newSelectedTrack = targetTrack;
            }
        }
    }

    selectedTrackIndex = newSelectedTrack;
    selectedClipIndex = newSelectedClip;
    arrangementView.setSelectedClip(selectedTrackIndex, selectedClipIndex);
    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds();
    updateTrackControlsFromSelection();
}

void MainComponent::showExportDialog()
{
    juce::Component::SafePointer<MainComponent> safeThis(this);
    juce::PopupMenu modeMenu;
    modeMenu.addItem(1, "One Track (selected)");
    modeMenu.addItem(2, "Stems (all tracks)");
    modeMenu.addItem(3, "All Tracks Mix");

    modeMenu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [safeThis](int modeId)
    {
        if (safeThis == nullptr || modeId == 0)
            return;

        juce::PopupMenu normMenu;
        normMenu.addItem(1, "Normalize");
        normMenu.addItem(2, "No Normalization");
        normMenu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(safeThis.getComponent()), [safeThis, modeId](int normId)
        {
            if (safeThis == nullptr || normId == 0)
                return;

            const bool normalize = (normId == 1);
            const int mode = modeId - 1;

            if (mode == 1)
            {
                safeThis->exportFileChooser = std::make_unique<juce::FileChooser>(
                    "Choose stems output folder",
                    juce::File{},
                    "*");

                auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;
                safeThis->exportFileChooser->launchAsync(flags, [safeThis, normalize](const juce::FileChooser& chooser)
                {
                    if (safeThis == nullptr)
                        return;
                    const auto dir = chooser.getResult();
                    if (dir.isDirectory())
                        safeThis->exportStems(dir, normalize);
                });
                return;
            }

            safeThis->exportFileChooser = std::make_unique<juce::FileChooser>(
                "Export WAV",
                juce::File{},
                "*.wav");

            auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
            safeThis->exportFileChooser->launchAsync(flags, [safeThis, mode, normalize](const juce::FileChooser& chooser)
            {
                if (safeThis == nullptr)
                    return;

                auto outFile = chooser.getResult();
                if (outFile == juce::File{})
                    return;
                if (!outFile.hasFileExtension(".wav"))
                    outFile = outFile.withFileExtension(".wav");

                if (mode == 0)
                    safeThis->exportOneTrack(outFile, normalize);
                else
                    safeThis->exportAllTracksMix(outFile, normalize);
            });
        });
    });
}

bool MainComponent::writeWavFile(const juce::File& file, const juce::AudioBuffer<float>& buffer) const
{
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
    if (stream == nullptr)
        return false;

    auto options = juce::AudioFormatWriterOptions{}
                       .withSampleRate(sampleRate)
                       .withNumChannels(buffer.getNumChannels())
                       .withBitsPerSample(exportBitDepth);

    auto writer = wav.createWriterFor(stream, options);
    if (writer == nullptr)
        return false;

    return writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

void MainComponent::normalizeBuffer(juce::AudioBuffer<float>& buffer) const
{
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = juce::jmax(peak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));

    if (peak <= 0.0f)
        return;

    const float target = 0.99f;
    const float gain = target / peak;
    buffer.applyGain(gain);
}

int64 MainComponent::getTrackEndSamples(const Track& track) const
{
    int64 endSample = 0;
    for (const auto& clip : track.getClips())
        endSample = juce::jmax(endSample, clip.startSample + track.getClipPlaybackLengthSamples(clip, sampleRate));
    return endSample;
}

void MainComponent::renderTrackToBuffer(Track& track, juce::AudioBuffer<float>& buffer) const
{
    const int blockSize = 1024;
    const int channels = buffer.getNumChannels();
    std::vector<float*> ptrs((size_t)channels, nullptr);

    for (int64 pos = 0; pos < buffer.getNumSamples(); pos += blockSize)
    {
        const int len = (int)juce::jmin<int64>(blockSize, buffer.getNumSamples() - pos);
        for (int ch = 0; ch < channels; ++ch)
            ptrs[(size_t)ch] = buffer.getWritePointer(ch, (int)pos);

        juce::AudioBuffer<float> view(ptrs.data(), channels, len);
        track.render(view, pos, sampleRate);
    }
}

void MainComponent::exportOneTrack(const juce::File& outputFile, bool normalize)
{
    Track track("Track");
    {
        juce::ScopedLock lock(engine.getTrackLock());
        if (selectedTrackIndex < 0 || selectedTrackIndex >= engine.getTracks().size())
            return;
        track = engine.getTracks().getReference(selectedTrackIndex);
    }

    const int64 length = juce::jmax<int64>(1, getTrackEndSamples(track));
    juce::AudioBuffer<float> buffer(2, (int)length);
    buffer.clear();
    renderTrackToBuffer(track, buffer);
    if (normalize)
        normalizeBuffer(buffer);

    const bool ok = writeWavFile(outputFile, buffer);
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::NoIcon,
                                           ok ? "Export Complete" : "Export Failed",
                                           ok ? ("Wrote " + outputFile.getFileName()) : "Could not write output file.");
}

void MainComponent::exportStems(const juce::File& outputDirectory, bool normalize)
{
    juce::Array<Track> tracks;
    {
        juce::ScopedLock lock(engine.getTrackLock());
        tracks = engine.getTracks();
    }

    bool allOk = true;
    for (int i = 0; i < tracks.size(); ++i)
    {
        auto track = tracks.getReference(i);
        const int64 length = juce::jmax<int64>(1, getTrackEndSamples(track));
        juce::AudioBuffer<float> buffer(2, (int)length);
        buffer.clear();
        renderTrackToBuffer(track, buffer);
        if (normalize)
            normalizeBuffer(buffer);

        const juce::String safeName = track.getName().retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_ ");
        const juce::File outFile = outputDirectory.getChildFile("stem_" + juce::String(i + 1) + "_" + safeName + ".wav");
        allOk = writeWavFile(outFile, buffer) && allOk;
    }

    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::NoIcon,
                                           allOk ? "Export Complete" : "Export Partial/Failed",
                                           allOk ? "All stems written." : "One or more stems could not be written.");
}

void MainComponent::exportAllTracksMix(const juce::File& outputFile, bool normalize)
{
    juce::Array<Track> tracks;
    int64 length = 1;
    {
        juce::ScopedLock lock(engine.getTrackLock());
        tracks = engine.getTracks();
        for (const auto& t : tracks)
            length = juce::jmax(length, getTrackEndSamples(t));
    }

    juce::AudioBuffer<float> mix(2, (int)length);
    mix.clear();
    for (int i = 0; i < tracks.size(); ++i)
    {
        auto t = tracks.getReference(i);
        renderTrackToBuffer(t, mix);
    }
    if (normalize)
        normalizeBuffer(mix);

    const bool ok = writeWavFile(outputFile, mix);
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::NoIcon,
                                           ok ? "Export Complete" : "Export Failed",
                                           ok ? ("Wrote " + outputFile.getFileName()) : "Could not write output file.");
}

void MainComponent::openBeatSlicerForSelection()
{
    TrackClip selectedClip;
    bool ok = false;
    {
        juce::ScopedLock lock(engine.getTrackLock());
        if (selectedTrackIndex >= 0 && selectedTrackIndex < engine.getTracks().size())
        {
            const auto& track = engine.getTracks().getReference(selectedTrackIndex);
            if (selectedClipIndex >= 0 && selectedClipIndex < track.getClips().size())
            {
                selectedClip = track.getClips().getReference(selectedClipIndex);
                ok = true;
            }
        }
    }

    if (!ok)
        return;

    beatSlicer.setClip(selectedClip);
    beatSlicer.setVisible(true);
    beatSlicer.toFront(true);
}

void MainComponent::applySlicingToSelectedClip(const BeatSlicingSettings& slicing)
{
    pushUndoState();
    juce::ScopedLock lock(engine.getTrackLock());
    if (selectedTrackIndex < 0 || selectedTrackIndex >= engine.getTracks().size())
        return;

    auto& track = engine.getTracks().getReference(selectedTrackIndex);
    if (selectedClipIndex < 0 || selectedClipIndex >= track.getClips().size())
        return;

    auto clip = track.getClips().getReference(selectedClipIndex);
    clip.slicing = slicing;
    track.updateClip(selectedClipIndex, clip);
}

void MainComponent::applyFitSettingsToSelectedClip(int fitLengthUnits, bool fitToSnapDivision)
{
    pushUndoState();
    {
        juce::ScopedLock lock(engine.getTrackLock());
        if (selectedTrackIndex < 0 || selectedTrackIndex >= engine.getTracks().size())
            return;

        auto& track = engine.getTracks().getReference(selectedTrackIndex);
        if (selectedClipIndex < 0 || selectedClipIndex >= track.getClips().size())
            return;

        auto clip = track.getClips().getReference(selectedClipIndex);
        clip.fitLengthUnits = juce::jmax(1, fitLengthUnits);
        clip.fitToSnapDivision = fitToSnapDivision;
        const int64 unitSamples = fitToSnapDivision ? track.getSnapLengthSamples(sampleRate)
                                                    : track.getBarLengthSamples(sampleRate);
        clip.lengthSamples = juce::jmax<int64>(1, unitSamples * (int64)clip.fitLengthUnits);
        track.updateClip(selectedClipIndex, clip);
    }

    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds();
    arrangementView.repaint();
}

void MainComponent::applyWarpToSelectedClip(const WarpCurve& curve, const juce::String&)
{
    pushUndoState();
    int trackIndex = -1;
    int clipIndex = -1;
    WarpCurve updatedCurve;

    {
        juce::ScopedLock lock(engine.getTrackLock());
        if (engine.getTracks().isEmpty())
            return;

        trackIndex = selectedTrackIndex;
        clipIndex = selectedClipIndex;

        if (trackIndex < 0 || trackIndex >= engine.getTracks().size())
            trackIndex = 0;

        auto& track = engine.getTracks().getReference(trackIndex);
        if (track.getClips().isEmpty())
            return;

        if (clipIndex < 0 || clipIndex >= track.getClips().size())
            clipIndex = 0;

        auto clip = track.getClips().getReference(clipIndex);
        TrackClip updated = clip;
        updated.warpCurve = curve;
        track.updateClip(clipIndex, updated);
        selectedTrackIndex = trackIndex;
        selectedClipIndex = clipIndex;
        updatedCurve = updated.warpCurve;
    }

    arrangementView.setSelectedClip(trackIndex, clipIndex);
    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds();
    curveEditor.setCurve(updatedCurve);
    warpPanel.setCurve(updatedCurve);
    {
        juce::ScopedLock lock(engine.getTrackLock());
        const auto& clip = engine.getTracks().getReference(trackIndex).getClips().getReference(clipIndex);
        warpPanel.setFitSettings(clip.fitLengthUnits, clip.fitToSnapDivision);
    }
}

void MainComponent::selectClip(int trackIndex, int clipIndex)
{
    WarpCurve selectedCurve;

    {
        juce::ScopedLock lock(engine.getTrackLock());
        if (trackIndex < 0 || trackIndex >= engine.getTracks().size())
            return;

        auto& track = engine.getTracks().getReference(trackIndex);
        if (clipIndex < 0 || clipIndex >= track.getClips().size())
            return;

        selectedTrackIndex = trackIndex;
        selectedClipIndex = clipIndex;
        selectedCurve = track.getClips().getReference(clipIndex).warpCurve;
    }

    arrangementView.setSelectedClip(trackIndex, clipIndex);
    curveEditor.setCurve(selectedCurve);
    warpPanel.setCurve(selectedCurve);
    {
        juce::ScopedLock lock(engine.getTrackLock());
        const auto& clip = engine.getTracks().getReference(trackIndex).getClips().getReference(clipIndex);
        warpPanel.setFitSettings(clip.fitLengthUnits, clip.fitToSnapDivision);
    }
    updateTrackControlsFromSelection();
    grabKeyboardFocus();
}

void MainComponent::selectTrack(int trackIndex)
{
    {
        juce::ScopedLock lock(engine.getTrackLock());
        if (trackIndex < 0 || trackIndex >= engine.getTracks().size())
            return;
        selectedTrackIndex = trackIndex;
        selectedClipIndex = -1;
    }

    arrangementView.setSelectedClip(trackIndex, -1);
    updateTrackControlsFromSelection();
    grabKeyboardFocus();
}

void MainComponent::moveClip(int sourceTrackIndex, int clipIndex, int targetTrackIndex, int64 newStartSample)
{
    pushUndoState();
    int finalTrack = -1;
    int finalClip = -1;
    WarpCurve selectedCurve;

    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (sourceTrackIndex < 0 || sourceTrackIndex >= tracks.size())
            return;
        if (targetTrackIndex < 0 || targetTrackIndex >= tracks.size())
            return;

        auto& sourceTrack = tracks.getReference(sourceTrackIndex);
        if (clipIndex < 0 || clipIndex >= sourceTrack.getClips().size())
            return;

        TrackClip moved = sourceTrack.getClips().getReference(clipIndex);
        moved.startSample = tracks.getReference(targetTrackIndex).snapSampleToGrid(newStartSample, sampleRate);
        {
            const auto& target = tracks.getReference(targetTrackIndex);
            const int units = juce::jmax(1, moved.fitLengthUnits);
            const int64 unitSamples = moved.fitToSnapDivision
                                          ? target.getSnapLengthSamples(sampleRate)
                                          : target.getBarLengthSamples(sampleRate);
            moved.lengthSamples = juce::jmax<int64>(1, unitSamples * (int64)units);
        }

        if (targetTrackIndex == sourceTrackIndex)
        {
            sourceTrack.updateClip(clipIndex, moved);
            finalTrack = sourceTrackIndex;
            finalClip = clipIndex;
        }
        else
        {
            juce::Array<TrackClip> rebuilt;
            const auto& sourceClips = sourceTrack.getClips();
            for (int i = 0; i < sourceClips.size(); ++i)
            {
                if (i != clipIndex)
                    rebuilt.add(sourceClips.getReference(i));
            }
            sourceTrack.clear();
            for (const auto& c : rebuilt)
                sourceTrack.addClip(c);

            auto& targetTrack = tracks.getReference(targetTrackIndex);
            targetTrack.addClip(moved);
            finalTrack = targetTrackIndex;
            finalClip = targetTrack.getClips().size() - 1;
        }

        selectedTrackIndex = finalTrack;
        selectedClipIndex = finalClip;
        selectedCurve = moved.warpCurve;
    }

    arrangementView.setSelectedClip(finalTrack, finalClip);
    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds();
    curveEditor.setCurve(selectedCurve);
    warpPanel.setCurve(selectedCurve);
    {
        juce::ScopedLock lock(engine.getTrackLock());
        const auto& clip = engine.getTracks().getReference(finalTrack).getClips().getReference(finalClip);
        warpPanel.setFitSettings(clip.fitLengthUnits, clip.fitToSnapDivision);
    }
    updateTrackControlsFromSelection();
}

void MainComponent::splitClip(int trackIndex, int clipIndex, int64 splitSample)
{
    pushUndoState();
    int newSelectedTrack = -1;
    int newSelectedClip = -1;
    WarpCurve selectedCurve = WarpCurve::linear();

    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;
        auto& track = tracks.getReference(trackIndex);
        if (clipIndex < 0 || clipIndex >= track.getClips().size())
            return;

        const auto original = track.getClips().getReference(clipIndex);
        const int64 clipLength = track.getClipPlaybackLengthSamples(original, sampleRate);
        const int64 clipStart = original.startSample;
        const int64 clipEnd = clipStart + clipLength;
        const int64 minLen = juce::jmax<int64>(128, (int64)(sampleRate * 0.01));
        if (splitSample <= clipStart + minLen || splitSample >= clipEnd - minLen)
            return;

        const double t = juce::jlimit(0.0, 1.0, (double)(splitSample - clipStart) / (double)juce::jmax<int64>(1, clipLength));
        const float srcStart = juce::jlimit(0.0f, 1.0f, original.sourceStartNorm);
        const float srcEnd = juce::jlimit(srcStart, 1.0f, original.sourceEndNorm);
        const float srcSplit = juce::jlimit(srcStart, srcEnd, srcStart + (srcEnd - srcStart) * (float)t);

        TrackClip left = original;
        left.lengthSamples = juce::jmax<int64>(1, splitSample - clipStart);
        left.sourceStartNorm = srcStart;
        left.sourceEndNorm = srcSplit;
        left.fadeOutNorm = 0.0f;

        TrackClip right = original;
        right.startSample = splitSample;
        right.lengthSamples = juce::jmax<int64>(1, clipEnd - splitSample);
        right.sourceStartNorm = srcSplit;
        right.sourceEndNorm = srcEnd;
        right.fadeInNorm = 0.0f;

        track.updateClip(clipIndex, left);
        track.addClip(right);
        newSelectedTrack = trackIndex;
        newSelectedClip = track.getClips().size() - 1;
        selectedCurve = right.warpCurve;
    }

    selectedTrackIndex = newSelectedTrack;
    selectedClipIndex = newSelectedClip;
    arrangementView.setSelectedClip(newSelectedTrack, newSelectedClip);
    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds();
    curveEditor.setCurve(selectedCurve);
    warpPanel.setCurve(selectedCurve);
    {
        juce::ScopedLock lock(engine.getTrackLock());
        const auto& clip = engine.getTracks().getReference(newSelectedTrack).getClips().getReference(newSelectedClip);
        warpPanel.setFitSettings(clip.fitLengthUnits, clip.fitToSnapDivision);
    }
    updateTrackControlsFromSelection();
}

void MainComponent::trimClip(int trackIndex, int clipIndex, int64 newStartSample, int64 newLengthSamples, float newSourceStartNorm, float newSourceEndNorm)
{
    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;
        auto& track = tracks.getReference(trackIndex);
        if (clipIndex < 0 || clipIndex >= track.getClips().size())
            return;

        auto clip = track.getClips().getReference(clipIndex);
        clip.startSample = juce::jmax<int64>(0, newStartSample);
        clip.lengthSamples = juce::jmax<int64>(1, newLengthSamples);
        clip.sourceStartNorm = juce::jlimit(0.0f, 1.0f, newSourceStartNorm);
        clip.sourceEndNorm = juce::jlimit(clip.sourceStartNorm, 1.0f, newSourceEndNorm);
        track.updateClip(clipIndex, clip);
        selectedTrackIndex = trackIndex;
        selectedClipIndex = clipIndex;
    }

    arrangementView.setSelectedClip(trackIndex, clipIndex);
    arrangementView.refreshTrackControls();
    arrangementView.repaint();
}

void MainComponent::deleteClip(int trackIndex, int clipIndex)
{
    pushUndoState();
    int nextTrack = trackIndex;
    int nextClip = -1;

    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;
        auto& track = tracks.getReference(trackIndex);
        const auto& oldClips = track.getClips();
        if (clipIndex < 0 || clipIndex >= oldClips.size())
            return;

        juce::Array<TrackClip> rebuilt;
        for (int i = 0; i < oldClips.size(); ++i)
            if (i != clipIndex)
                rebuilt.add(oldClips.getReference(i));
        track.clear();
        for (const auto& c : rebuilt)
            track.addClip(c);

        if (!track.getClips().isEmpty())
            nextClip = juce::jlimit(0, track.getClips().size() - 1, clipIndex);
    }

    selectedTrackIndex = nextTrack;
    selectedClipIndex = nextClip;
    arrangementView.setSelectedClip(nextTrack, nextClip);
    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds();
    updateTrackControlsFromSelection();
}

void MainComponent::glueClips(const juce::Array<ArrangementView::ClipRef>& clipRefs)
{
    if (clipRefs.size() < 2)
        return;

    pushUndoState();
    int trackIndex = -1;
    int newClipIndex = -1;
    WarpCurve curve = WarpCurve::linear();

    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        for (const auto& r : clipRefs)
        {
            if (trackIndex < 0)
                trackIndex = r.trackIndex;
            if (r.trackIndex != trackIndex)
                return;
        }
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;

        auto& track = tracks.getReference(trackIndex);
        const auto& clips = track.getClips();

        juce::Array<int> idx;
        for (const auto& r : clipRefs)
        {
            if (r.clipIndex >= 0 && r.clipIndex < clips.size())
                idx.addIfNotAlreadyThere(r.clipIndex);
        }
        if (idx.size() < 2)
            return;

        TrackClip base = clips.getReference(idx.getReference(0));
        const juce::String baseSource = base.sourceFilePath;
        int64 minStart = base.startSample;
        int64 maxEnd = base.startSample + track.getClipPlaybackLengthSamples(base, sampleRate);
        float minSrc = base.sourceStartNorm;
        float maxSrc = base.sourceEndNorm;

        for (int i = 1; i < idx.size(); ++i)
        {
            const auto& c = clips.getReference(idx.getReference(i));
            if (c.sourceFilePath != baseSource)
                return;
            minStart = juce::jmin(minStart, c.startSample);
            maxEnd = juce::jmax(maxEnd, c.startSample + track.getClipPlaybackLengthSamples(c, sampleRate));
            minSrc = juce::jmin(minSrc, c.sourceStartNorm);
            maxSrc = juce::jmax(maxSrc, c.sourceEndNorm);
        }

        base.startSample = minStart;
        base.lengthSamples = juce::jmax<int64>(1, maxEnd - minStart);
        base.sourceStartNorm = juce::jlimit(0.0f, 1.0f, minSrc);
        base.sourceEndNorm = juce::jlimit(base.sourceStartNorm, 1.0f, maxSrc);
        base.fadeInNorm = 0.0f;
        base.fadeOutNorm = 0.0f;

        juce::Array<TrackClip> rebuilt;
        for (int i = 0; i < clips.size(); ++i)
            if (!idx.contains(i))
                rebuilt.add(clips.getReference(i));

        rebuilt.add(base);
        track.clear();
        for (const auto& c : rebuilt)
            track.addClip(c);

        newClipIndex = track.getClips().size() - 1;
        curve = base.warpCurve;
    }

    selectedTrackIndex = trackIndex;
    selectedClipIndex = newClipIndex;
    arrangementView.setSelectedClip(trackIndex, newClipIndex);
    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds();
    curveEditor.setCurve(curve);
    warpPanel.setCurve(curve);
    updateTrackControlsFromSelection();
}

void MainComponent::editTrackName(int trackIndex, const juce::String& name)
{
    pushUndoState();
    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;
        tracks.getReference(trackIndex).setName(name);
    }
    arrangementView.repaint();
}

void MainComponent::editTrackTempo(int trackIndex, double tempoBpm)
{
    pushUndoState();
    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;
        auto& track = tracks.getReference(trackIndex);
        const int64 oldBeat = juce::jmax<int64>(1, track.getBeatLengthSamples(sampleRate));
        const bool hasM0 = track.hasLoopMarker(0);
        const bool hasM1 = track.hasLoopMarker(1);
        const int64 m0 = track.getLoopMarker(0);
        const int64 m1 = track.getLoopMarker(1);

        track.setTempoBpm(tempoBpm);

        const int64 newBeat = juce::jmax<int64>(1, track.getBeatLengthSamples(sampleRate));
        const double ratio = (double)newBeat / (double)oldBeat;
        if (hasM0) track.setLoopMarker(0, (int64)std::llround((double)m0 * ratio));
        if (hasM1) track.setLoopMarker(1, (int64)std::llround((double)m1 * ratio));
        selectedTrackIndex = trackIndex;
    }
    refreshClipFitLengthsForTrack(trackIndex);
    updateTrackControlsFromSelection();
    arrangementView.repaint();
}

void MainComponent::editTrackTimeSignature(int trackIndex, int numerator, int denominator)
{
    pushUndoState();
    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;
        auto& track = tracks.getReference(trackIndex);
        const int64 oldBeat = juce::jmax<int64>(1, track.getBeatLengthSamples(sampleRate));
        const bool hasM0 = track.hasLoopMarker(0);
        const bool hasM1 = track.hasLoopMarker(1);
        const int64 m0 = track.getLoopMarker(0);
        const int64 m1 = track.getLoopMarker(1);

        track.setTimeSignature(numerator, denominator);

        const int64 newBeat = juce::jmax<int64>(1, track.getBeatLengthSamples(sampleRate));
        const double ratio = (double)newBeat / (double)oldBeat;
        if (hasM0) track.setLoopMarker(0, (int64)std::llround((double)m0 * ratio));
        if (hasM1) track.setLoopMarker(1, (int64)std::llround((double)m1 * ratio));
        selectedTrackIndex = trackIndex;
    }
    refreshClipFitLengthsForTrack(trackIndex);
    updateTrackControlsFromSelection();
    arrangementView.repaint();
}

void MainComponent::editTrackLoopMarker(int trackIndex, int markerIndex, int64 samplePosition, bool enabled)
{
    pushUndoState();
    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;

        auto& track = tracks.getReference(trackIndex);
        if (!enabled)
        {
            track.clearLoopMarker(markerIndex);
        }
        else
        {
            track.setLoopMarker(markerIndex, juce::jmax<int64>(0, samplePosition));
        }
        selectedTrackIndex = trackIndex;
    }
    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds();
}

void MainComponent::editTrackZoomX(int trackIndex, double zoomValue)
{
    pushUndoState();
    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;
        tracks.getReference(trackIndex).setZoomX(zoomValue);
        selectedTrackIndex = trackIndex;
    }
    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds();
}

void MainComponent::editTrackZoomY(int trackIndex, double zoomValue)
{
    pushUndoState();
    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;
        tracks.getReference(trackIndex).setZoomY(zoomValue);
        selectedTrackIndex = trackIndex;
    }
    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds();
}

void MainComponent::editTrackVolume(int trackIndex, double volumeValue)
{
    pushUndoState();
    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;
        tracks.getReference(trackIndex).setVolume(volumeValue);
        selectedTrackIndex = trackIndex;
    }
    arrangementView.refreshTrackControls();
}

void MainComponent::editTrackPan(int trackIndex, double panValue)
{
    pushUndoState();
    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;
        tracks.getReference(trackIndex).setPan(panValue);
        selectedTrackIndex = trackIndex;
    }
    arrangementView.refreshTrackControls();
}

void MainComponent::editClipGain(int trackIndex, int clipIndex, float gain)
{
    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;
        auto& clips = tracks.getReference(trackIndex).getClips();
        if (clipIndex < 0 || clipIndex >= clips.size())
            return;

        auto clip = clips.getReference(clipIndex);
        clip.gain = juce::jlimit(0.0f, 2.0f, gain);
        tracks.getReference(trackIndex).updateClip(clipIndex, clip);
        selectedTrackIndex = trackIndex;
        selectedClipIndex = clipIndex;
    }
    arrangementView.repaint();
}

void MainComponent::editClipFade(int trackIndex, int clipIndex, float fadeInNorm, float fadeOutNorm)
{
    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;
        auto& clips = tracks.getReference(trackIndex).getClips();
        if (clipIndex < 0 || clipIndex >= clips.size())
            return;

        auto clip = clips.getReference(clipIndex);
        clip.fadeInNorm = juce::jlimit(0.0f, 0.98f, fadeInNorm);
        clip.fadeOutNorm = juce::jlimit(0.0f, 0.98f - clip.fadeInNorm, fadeOutNorm);
        tracks.getReference(trackIndex).updateClip(clipIndex, clip);
        selectedTrackIndex = trackIndex;
        selectedClipIndex = clipIndex;
    }
    arrangementView.repaint();
}

void MainComponent::editClipFit(int trackIndex, int clipIndex, int fitLengthUnits, bool fitToSnapDivision)
{
    pushUndoState();
    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;
        auto& track = tracks.getReference(trackIndex);
        auto& clips = track.getClips();
        if (clipIndex < 0 || clipIndex >= clips.size())
            return;

        auto clip = clips.getReference(clipIndex);
        clip.fitLengthUnits = juce::jmax(1, fitLengthUnits);
        clip.fitToSnapDivision = fitToSnapDivision;
        const int64 unitSamples = fitToSnapDivision ? track.getSnapLengthSamples(sampleRate)
                                                    : track.getBarLengthSamples(sampleRate);
        clip.lengthSamples = juce::jmax<int64>(1, unitSamples * (int64)clip.fitLengthUnits);
        track.updateClip(clipIndex, clip);
        selectedTrackIndex = trackIndex;
        selectedClipIndex = clipIndex;
    }

    arrangementView.setSelectedClip(trackIndex, clipIndex);
    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds();
    {
        juce::ScopedLock lock(engine.getTrackLock());
        const auto& clip = engine.getTracks().getReference(trackIndex).getClips().getReference(clipIndex);
        warpPanel.setFitSettings(clip.fitLengthUnits, clip.fitToSnapDivision);
    }
    updateTrackControlsFromSelection();
}

void MainComponent::editTrackAutomationPoints(int trackIndex, bool isVolumeLane, const juce::Array<TrackAutomationPoint>& points)
{
    pushUndoState();
    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;

        auto& track = tracks.getReference(trackIndex);
        if (isVolumeLane)
            track.setVolumeAutomation(points);
        else
            track.setPanAutomation(points);
        selectedTrackIndex = trackIndex;
    }
    arrangementView.repaint();
}

void MainComponent::deleteTrack(int trackIndex)
{
    pushUndoState();
    int newSelectedTrack = -1;

    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;

        tracks.remove(trackIndex);
        selectedClipIndex = -1;
        if (!tracks.isEmpty())
            newSelectedTrack = juce::jlimit(0, tracks.size() - 1, trackIndex);
        selectedTrackIndex = newSelectedTrack;
    }

    arrangementView.setSelectedClip(newSelectedTrack, -1);
    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds();
    updateTrackControlsFromSelection();
    grabKeyboardFocus();
}

void MainComponent::updateTrackControlsFromSelection()
{
    juce::ScopedValueSetter<bool> guard(suppressTrackControlCallbacks, true);

    juce::ScopedLock lock(engine.getTrackLock());
    if (selectedTrackIndex < 0 || selectedTrackIndex >= engine.getTracks().size())
        return;

    const auto& track = engine.getTracks().getReference(selectedTrackIndex);
    tempoSlider.setValue(track.getTempoBpm(), juce::dontSendNotification);
    timeSigNumBox.setSelectedId(track.getTimeSigNumerator(), juce::dontSendNotification);
    timeSigDenBox.setSelectedId(track.getTimeSigDenominator(), juce::dontSendNotification);
    snapToggle.setToggleState(track.isSnapEnabled(), juce::dontSendNotification);
    snapDivisionBox.setSelectedId(snapDivisionToId(track.getSnapDivision()), juce::dontSendNotification);
}

void MainComponent::applyTrackTimingFromControls()
{
    if (suppressTrackControlCallbacks)
        return;
    pushUndoState();

    {
        juce::ScopedLock lock(engine.getTrackLock());
        if (selectedTrackIndex < 0 || selectedTrackIndex >= engine.getTracks().size())
            return;

        auto& track = engine.getTracks().getReference(selectedTrackIndex);
        const int64 oldBeat = juce::jmax<int64>(1, track.getBeatLengthSamples(sampleRate));
        const bool hasM0 = track.hasLoopMarker(0);
        const bool hasM1 = track.hasLoopMarker(1);
        const int64 m0 = track.getLoopMarker(0);
        const int64 m1 = track.getLoopMarker(1);
        track.setTempoBpm(tempoSlider.getValue());

        const int num = timeSigNumBox.getSelectedId() > 0 ? timeSigNumBox.getSelectedId() : track.getTimeSigNumerator();
        const int den = timeSigDenBox.getSelectedId() > 0 ? timeSigDenBox.getSelectedId() : track.getTimeSigDenominator();
        track.setTimeSignature(num, den);
        track.setSnapEnabled(snapToggle.getToggleState());
        track.setSnapDivision(idToSnapDivision(snapDivisionBox.getSelectedId()));

        const int64 newBeat = juce::jmax<int64>(1, track.getBeatLengthSamples(sampleRate));
        const double ratio = (double)newBeat / (double)oldBeat;
        if (hasM0) track.setLoopMarker(0, (int64)std::llround((double)m0 * ratio));
        if (hasM1) track.setLoopMarker(1, (int64)std::llround((double)m1 * ratio));
    }

    refreshClipFitLengthsForTrack(selectedTrackIndex);
    arrangementView.repaint();
}

void MainComponent::refreshClipFitLengthsForTrack(int trackIndex)
{
    {
        juce::ScopedLock lock(engine.getTrackLock());
        auto& tracks = engine.getTracks();
        if (trackIndex < 0 || trackIndex >= tracks.size())
            return;

        auto& track = tracks.getReference(trackIndex);
        auto clips = track.getClips();
        for (int i = 0; i < clips.size(); ++i)
        {
            auto clip = clips.getReference(i);
            const int units = juce::jmax(1, clip.fitLengthUnits);
            const int64 unitSamples = clip.fitToSnapDivision
                                          ? track.getSnapLengthSamples(sampleRate)
                                          : track.getBarLengthSamples(sampleRate);
            const int64 nextLen = juce::jmax<int64>(1, unitSamples * (int64)units);
            clip.lengthSamples = nextLen;
            track.updateClip(i, clip);
        }
    }
    arrangementView.refreshTrackControls();
    updateArrangementViewportBounds();
}
