#include "BeatSlicerComponent.h"
#include <algorithm>
#include <cmath>

void BeatSlicerComponent::SliceWaveformView::setSample(const std::shared_ptr<Sample>& sampleIn)
{
    sample = sampleIn;
    repaint();
}

void BeatSlicerComponent::SliceWaveformView::setSlices(const juce::Array<BeatSlice>& slicesIn)
{
    slices = slicesIn;
    repaint();
}

double BeatSlicerComponent::SliceWaveformView::xToNorm(float x) const
{
    const auto bounds = getLocalBounds().toFloat().reduced(8.0f);
    if (bounds.getWidth() <= 1.0f)
        return 0.0;
    return juce::jlimit(0.0, 1.0, (double)((x - bounds.getX()) / bounds.getWidth()));
}

float BeatSlicerComponent::SliceWaveformView::normToX(double norm) const
{
    const auto bounds = getLocalBounds().toFloat().reduced(8.0f);
    return bounds.getX() + (float)juce::jlimit(0.0, 1.0, norm) * bounds.getWidth();
}

void BeatSlicerComponent::SliceWaveformView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(16, 18, 22));
    auto area = getLocalBounds().toFloat().reduced(8.0f);
    g.setColour(juce::Colour(26, 30, 36));
    g.fillRoundedRectangle(area, 6.0f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(area, 6.0f, 1.0f);

    if (sample != nullptr && sample->getNumSamples() > 0)
    {
        const int width = juce::jmax(1, (int)area.getWidth());
        const float midY = area.getCentreY();
        const float radius = area.getHeight() * 0.45f;
        const int totalSamples = sample->getNumSamples();

        g.setColour(juce::Colours::white.withAlpha(0.65f));
        for (int x = 0; x < width; ++x)
        {
            const int startSample = (int)(((double)x / (double)width) * (double)totalSamples);
            const int endSample = juce::jmin(totalSamples - 1, (int)(((double)(x + 1) / (double)width) * (double)totalSamples));

            float minV = 1.0f;
            float maxV = -1.0f;
            for (int sIdx = startSample; sIdx <= endSample; ++sIdx)
            {
                const float v = sample->getSampleAt(0, (double)sIdx);
                minV = std::min(minV, v);
                maxV = std::max(maxV, v);
            }

            if (minV > maxV)
                continue;

            const float drawX = area.getX() + (float)x;
            g.drawLine(drawX, midY - maxV * radius, drawX, midY - minV * radius);
        }
    }
    else
    {
        g.setColour(juce::Colours::white.withAlpha(0.55f));
        g.drawText("No waveform available for this clip", area.toNearestInt(), juce::Justification::centred, false);
    }

    g.setColour(juce::Colour(242, 205, 110).withAlpha(0.85f));
    for (int i = 1; i < slices.size(); ++i)
    {
        const float x = normToX(slices.getReference(i).startNorm);
        g.drawLine(x, area.getY(), x, area.getBottom(), 2.0f);
    }
}

void BeatSlicerComponent::SliceWaveformView::mouseDown(const juce::MouseEvent& e)
{
    draggingBoundary = -1;
    if (slices.size() < 2)
        return;

    const float px = e.position.x;
    for (int i = 1; i < slices.size(); ++i)
    {
        const float bx = normToX(slices.getReference(i).startNorm);
        if (std::abs(px - bx) < 8.0f)
        {
            draggingBoundary = i;
            break;
        }
    }
}

void BeatSlicerComponent::SliceWaveformView::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingBoundary <= 0 || draggingBoundary >= slices.size())
        return;
    if (!boundaryMoved)
        return;

    boundaryMoved(draggingBoundary, xToNorm(e.position.x));
}

BeatSlicerComponent::BeatSlicerComponent()
{
    addAndMakeVisible(enableSlicingButton);
    addAndMakeVisible(modeBox);
    addAndMakeVisible(sliceSelectBox);
    addAndMakeVisible(probabilitySlider);
    addAndMakeVisible(repeatCountSlider);
    addAndMakeVisible(repeatProbabilitySlider);
    addAndMakeVisible(modeLabel);
    addAndMakeVisible(sliceLabel);
    addAndMakeVisible(playProbabilityLabel);
    addAndMakeVisible(repeatCountLabel);
    addAndMakeVisible(repeatProbabilityLabel);
    addAndMakeVisible(helpLabel);
    addAndMakeVisible(autoDetectButton);
    addAndMakeVisible(addSliceButton);
    addAndMakeVisible(removeSliceButton);
    addAndMakeVisible(closeButton);
    addAndMakeVisible(waveformView);

    addSliceButton.addListener(this);
    removeSliceButton.addListener(this);
    closeButton.addListener(this);
    autoDetectButton.addListener(this);

    modeBox.addItem("Sequential (original order)", (int)SlicePlaybackMode::sequential + 1);
    modeBox.addItem("Rotate (shift each play)", (int)SlicePlaybackMode::rotate + 1);
    modeBox.addItem("Random (shuffle each play)", (int)SlicePlaybackMode::random + 1);
    modeBox.setTooltip("How slice order is chosen each time the clip is triggered.");

    probabilitySlider.setRange(0.0, 1.0, 0.01);
    probabilitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 20);
    probabilitySlider.setNumDecimalPlacesToDisplay(2);
    probabilitySlider.setTooltip("Chance that this slice will play when reached. 0 = never, 1 = always.");

    repeatCountSlider.setRange(1.0, 8.0, 1.0);
    repeatCountSlider.setSliderStyle(juce::Slider::IncDecButtons);
    repeatCountSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 20);
    repeatCountSlider.setTooltip("How many sub-repeats to split this slice into.");

    repeatProbabilitySlider.setRange(0.0, 1.0, 0.01);
    repeatProbabilitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 56, 20);
    repeatProbabilitySlider.setNumDecimalPlacesToDisplay(2);
    repeatProbabilitySlider.setTooltip("Probability that each extra ratchet repeat will fire.");
    autoDetectButton.setTooltip("Analyze waveform transients and create slice boundaries automatically.");

    modeLabel.setText("Playback Mode", juce::dontSendNotification);
    sliceLabel.setText("Editing Slice", juce::dontSendNotification);
    playProbabilityLabel.setText("Slice Play Probability", juce::dontSendNotification);
    repeatCountLabel.setText("Ratcheting Repeats", juce::dontSendNotification);
    repeatProbabilityLabel.setText("Repeat Probability", juce::dontSendNotification);
    helpLabel.setText("Drag vertical lines in the waveform to move slice boundaries. Press Close to return to timeline.", juce::dontSendNotification);
    helpLabel.setJustificationType(juce::Justification::centredLeft);

    for (auto* lbl : {&modeLabel, &sliceLabel, &playProbabilityLabel, &repeatCountLabel, &repeatProbabilityLabel, &helpLabel})
    {
        lbl->setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
    }

    modeLabel.setJustificationType(juce::Justification::centredLeft);
    sliceLabel.setJustificationType(juce::Justification::centredLeft);
    playProbabilityLabel.setJustificationType(juce::Justification::centredLeft);
    repeatCountLabel.setJustificationType(juce::Justification::centredLeft);
    repeatProbabilityLabel.setJustificationType(juce::Justification::centredLeft);

    waveformView.onBoundaryMoved([this](int boundaryIndex, double newNorm)
    {
        handleBoundaryMove(boundaryIndex, newNorm);
    });

    enableSlicingButton.onClick = [this]()
    {
        if (suppressCallbacks)
            return;
        state.enabled = enableSlicingButton.getToggleState();
        emitChanged();
    };

    modeBox.onChange = [this]()
    {
        if (suppressCallbacks)
            return;
        state.mode = (SlicePlaybackMode)(modeBox.getSelectedId() - 1);
        emitChanged();
    };

    sliceSelectBox.onChange = [this]()
    {
        if (suppressCallbacks)
            return;
        selectedSlice = juce::jmax(0, sliceSelectBox.getSelectedId() - 1);
        refreshSelectedSliceUi();
    };

    probabilitySlider.onValueChange = [this]()
    {
        if (suppressCallbacks)
            return;
        if (selectedSlice >= 0 && selectedSlice < state.slices.size())
        {
            state.slices.getReference(selectedSlice).playProbability = probabilitySlider.getValue();
            emitChanged();
        }
    };

    repeatCountSlider.onValueChange = [this]()
    {
        if (suppressCallbacks)
            return;
        if (selectedSlice >= 0 && selectedSlice < state.slices.size())
        {
            state.slices.getReference(selectedSlice).ratchetRepeats = (int)repeatCountSlider.getValue();
            emitChanged();
        }
    };

    repeatProbabilitySlider.onValueChange = [this]()
    {
        if (suppressCallbacks)
            return;
        if (selectedSlice >= 0 && selectedSlice < state.slices.size())
        {
            state.slices.getReference(selectedSlice).repeatProbability = repeatProbabilitySlider.getValue();
            emitChanged();
        }
    };
}

void BeatSlicerComponent::setClip(const TrackClip& clip)
{
    sample = clip.sample;
    state = clip.slicing;
    ensureSlicesInitialized();
    selectedSlice = juce::jlimit(0, juce::jmax(0, state.slices.size() - 1), selectedSlice);
    refreshUiFromState();
}

void BeatSlicerComponent::ensureSlicesInitialized()
{
    if (!state.slices.isEmpty())
        return;

    for (int i = 0; i < 4; ++i)
    {
        BeatSlice s;
        s.startNorm = (double)i / 4.0;
        s.endNorm = (double)(i + 1) / 4.0;
        s.playProbability = 1.0;
        s.ratchetRepeats = 1;
        s.repeatProbability = 1.0;
        state.slices.add(s);
    }
}

void BeatSlicerComponent::refreshUiFromState()
{
    suppressCallbacks = true;
    waveformView.setSample(sample);
    waveformView.setSlices(state.slices);

    enableSlicingButton.setToggleState(state.enabled, juce::dontSendNotification);
    modeBox.setSelectedId((int)state.mode + 1, juce::dontSendNotification);

    sliceSelectBox.clear(juce::dontSendNotification);
    for (int i = 0; i < state.slices.size(); ++i)
        sliceSelectBox.addItem("Slice " + juce::String(i + 1), i + 1);
    if (state.slices.isEmpty())
        sliceSelectBox.setSelectedId(0, juce::dontSendNotification);
    else
        sliceSelectBox.setSelectedId(selectedSlice + 1, juce::dontSendNotification);

    refreshSelectedSliceUi();
    suppressCallbacks = false;
}

void BeatSlicerComponent::refreshSelectedSliceUi()
{
    suppressCallbacks = true;
    if (selectedSlice >= 0 && selectedSlice < state.slices.size())
    {
        const auto& s = state.slices.getReference(selectedSlice);
        probabilitySlider.setValue(s.playProbability, juce::dontSendNotification);
        repeatCountSlider.setValue((double)s.ratchetRepeats, juce::dontSendNotification);
        repeatProbabilitySlider.setValue(s.repeatProbability, juce::dontSendNotification);
    }
    suppressCallbacks = false;
}

void BeatSlicerComponent::emitChanged()
{
    waveformView.setSlices(state.slices);
    if (slicingChanged)
        slicingChanged(state);
}

void BeatSlicerComponent::addSlice()
{
    ensureSlicesInitialized();

    if (selectedSlice < 0 || selectedSlice >= state.slices.size())
        selectedSlice = 0;

    auto& s = state.slices.getReference(selectedSlice);
    const double mid = (s.startNorm + s.endNorm) * 0.5;

    BeatSlice right = s;
    right.startNorm = mid;
    s.endNorm = mid;

    state.slices.insert(selectedSlice + 1, right);
    selectedSlice = selectedSlice + 1;
    refreshUiFromState();
    emitChanged();
}

void BeatSlicerComponent::removeSlice()
{
    if (state.slices.size() <= 1)
        return;
    if (selectedSlice < 0 || selectedSlice >= state.slices.size())
        return;

    const int removeIndex = selectedSlice;
    if (removeIndex == 0)
    {
        state.slices.getReference(1).startNorm = 0.0;
    }
    else
    {
        const double removedEnd = state.slices.getReference(removeIndex).endNorm;
        state.slices.getReference(removeIndex - 1).endNorm = removedEnd;
    }

    state.slices.remove(removeIndex);
    selectedSlice = juce::jlimit(0, state.slices.size() - 1, selectedSlice - 1);
    refreshUiFromState();
    emitChanged();
}

void BeatSlicerComponent::handleBoundaryMove(int boundaryIndex, double newNorm)
{
    if (boundaryIndex <= 0 || boundaryIndex >= state.slices.size())
        return;

    const double minNorm = state.slices.getReference(boundaryIndex - 1).startNorm + 0.01;
    const double maxNorm = state.slices.getReference(boundaryIndex).endNorm - 0.01;
    const double t = juce::jlimit(minNorm, maxNorm, newNorm);

    state.slices.getReference(boundaryIndex - 1).endNorm = t;
    state.slices.getReference(boundaryIndex).startNorm = t;

    waveformView.setSlices(state.slices);
    emitChanged();
}

void BeatSlicerComponent::buttonClicked(juce::Button* button)
{
    if (button == &autoDetectButton)
        autoDetectBeats();
    else if (button == &addSliceButton)
        addSlice();
    else if (button == &removeSliceButton)
        removeSlice();
    else if (button == &closeButton)
    {
        if (closeRequested)
            closeRequested();
    }
}

void BeatSlicerComponent::autoDetectBeats()
{
    if (sample == nullptr || sample->getNumSamples() <= 0)
        return;

    const int totalSamples = sample->getNumSamples();
    const double sr = juce::jmax(1.0, sample->getSampleRate());

    // 10 ms analysis hop gives enough temporal precision for slicing.
    const int hop = juce::jmax(32, (int)std::round(sr * 0.010));
    const int frame = juce::jmax(hop * 2, (int)std::round(sr * 0.020));
    const int frameCount = juce::jmax(1, totalSamples / hop);

    std::vector<double> env((size_t)frameCount, 0.0);
    std::vector<double> flux((size_t)frameCount, 0.0);

    for (int i = 0; i < frameCount; ++i)
    {
        const int center = i * hop;
        const int start = juce::jmax(0, center - frame / 2);
        const int end = juce::jmin(totalSamples - 1, start + frame);
        double sumSq = 0.0;
        int n = 0;
        for (int sIdx = start; sIdx < end; ++sIdx)
        {
            const double v = (double)sample->getSampleAt(0, (double)sIdx);
            sumSq += v * v;
            ++n;
        }
        env[(size_t)i] = n > 0 ? std::sqrt(sumSq / (double)n) : 0.0;
    }

    for (int i = 1; i < frameCount; ++i)
        flux[(size_t)i] = juce::jmax(0.0, env[(size_t)i] - env[(size_t)(i - 1)]);

    const int threshWin = 12;
    const int minGapFrames = juce::jmax(2, (int)std::round(0.080 * sr / (double)hop));
    const int maxSlices = 32;
    std::vector<int> onsets;
    onsets.push_back(0);

    int lastAccepted = -minGapFrames;
    for (int i = 2; i < frameCount - 2; ++i)
    {
        int a = juce::jmax(0, i - threshWin);
        int b = juce::jmin(frameCount - 1, i + threshWin);
        double mean = 0.0;
        for (int k = a; k <= b; ++k)
            mean += flux[(size_t)k];
        mean /= (double)juce::jmax(1, b - a + 1);

        // Adaptive threshold + local peak check.
        const double threshold = mean * 1.8;
        const bool localPeak = flux[(size_t)i] > flux[(size_t)(i - 1)] && flux[(size_t)i] >= flux[(size_t)(i + 1)];
        if (localPeak && flux[(size_t)i] > threshold && (i - lastAccepted) >= minGapFrames)
        {
            onsets.push_back(i * hop);
            lastAccepted = i;
            if ((int)onsets.size() >= maxSlices)
                break;
        }
    }
    onsets.push_back(totalSamples);

    // De-duplicate and enforce minimum slice length.
    const int minSliceSamples = juce::jmax(1, (int)std::round(sr * 0.040));
    std::vector<int> boundaries;
    boundaries.push_back(0);
    for (int pos : onsets)
    {
        pos = juce::jlimit(0, totalSamples, pos);
        if (pos - boundaries.back() >= minSliceSamples)
            boundaries.push_back(pos);
    }
    if (boundaries.back() != totalSamples)
        boundaries.push_back(totalSamples);

    if ((int)boundaries.size() < 3)
    {
        // Fallback: eight uniform slices when transients are weak.
        boundaries.clear();
        boundaries.push_back(0);
        for (int i = 1; i < 8; ++i)
            boundaries.push_back((int)std::round((double)i / 8.0 * (double)totalSamples));
        boundaries.push_back(totalSamples);
    }

    state.slices.clear();
    for (size_t i = 0; i + 1 < boundaries.size(); ++i)
    {
        BeatSlice s;
        s.startNorm = (double)boundaries[i] / (double)totalSamples;
        s.endNorm = (double)boundaries[i + 1] / (double)totalSamples;
        s.playProbability = 1.0;
        s.ratchetRepeats = 1;
        s.repeatProbability = 1.0;
        state.slices.add(s);
    }

    selectedSlice = 0;
    refreshUiFromState();
    emitChanged();
}

void BeatSlicerComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(8, 10, 12).withAlpha(0.96f));
    g.setColour(juce::Colour(30, 35, 42));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2.0f), 8.0f, 1.0f);
}

void BeatSlicerComponent::resized()
{
    auto area = getLocalBounds().reduced(10);
    auto titleRow = area.removeFromTop(28);
    closeButton.setBounds(titleRow.removeFromRight(86));
    enableSlicingButton.setBounds(titleRow.removeFromLeft(150));

    area.removeFromTop(6);
    auto modeRow = area.removeFromTop(24);
    modeLabel.setBounds(modeRow.removeFromLeft(108));
    modeBox.setBounds(modeRow.removeFromLeft(260));
    modeRow.removeFromLeft(8);
    autoDetectButton.setBounds(modeRow.removeFromLeft(140));
    modeRow.removeFromLeft(10);
    sliceLabel.setBounds(modeRow.removeFromLeft(92));
    sliceSelectBox.setBounds(modeRow.removeFromLeft(130));
    modeRow.removeFromLeft(8);
    addSliceButton.setBounds(modeRow.removeFromLeft(78));
    modeRow.removeFromLeft(4);
    removeSliceButton.setBounds(modeRow.removeFromLeft(78));

    area.removeFromTop(8);
    const int waveformH = juce::jmax(180, area.getHeight() / 2);
    waveformView.setBounds(area.removeFromTop(waveformH));

    area.removeFromTop(8);
    helpLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(8);

    const int colGap = 10;
    const int colW = (area.getWidth() - colGap * 2) / 3;
    auto c1 = area.removeFromLeft(colW);
    area.removeFromLeft(colGap);
    auto c2 = area.removeFromLeft(colW);
    area.removeFromLeft(colGap);
    auto c3 = area;

    playProbabilityLabel.setBounds(c1.removeFromTop(20));
    probabilitySlider.setBounds(c1.removeFromTop(30));

    repeatCountLabel.setBounds(c2.removeFromTop(20));
    repeatCountSlider.setBounds(c2.removeFromTop(30));

    repeatProbabilityLabel.setBounds(c3.removeFromTop(20));
    repeatProbabilitySlider.setBounds(c3.removeFromTop(30));
}
