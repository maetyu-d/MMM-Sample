#pragma once

#include <JuceHeader.h>
#include "Track.h"
#include <vector>

class BeatSlicerComponent : public juce::Component,
                            private juce::Button::Listener
{
public:
    using SlicingChanged = std::function<void(const BeatSlicingSettings&)>;

    BeatSlicerComponent();

    void setClip(const TrackClip& clip);
    void onSlicingChanged(SlicingChanged cb) { slicingChanged = std::move(cb); }
    void onClose(std::function<void()> cb) { closeRequested = std::move(cb); }

    void paint(juce::Graphics& g) override;
    void resized() override;

    void buttonClicked(juce::Button* button) override;

private:
    class SliceWaveformView : public juce::Component
    {
    public:
        using BoundaryMoved = std::function<void(int boundaryIndex, double newNorm)>;

        void setSample(const std::shared_ptr<Sample>& sampleIn);
        void setSlices(const juce::Array<BeatSlice>& slicesIn);
        void onBoundaryMoved(BoundaryMoved cb) { boundaryMoved = std::move(cb); }

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent&) override { draggingBoundary = -1; }

    private:
        double xToNorm(float x) const;
        float normToX(double norm) const;

        std::shared_ptr<Sample> sample;
        juce::Array<BeatSlice> slices;
        BoundaryMoved boundaryMoved;
        int draggingBoundary = -1;
    };

    void refreshUiFromState();
    void refreshSelectedSliceUi();
    void emitChanged();
    void ensureSlicesInitialized();
    void addSlice();
    void removeSlice();
    void handleBoundaryMove(int boundaryIndex, double newNorm);
    void autoDetectBeats();

    std::shared_ptr<Sample> sample;
    BeatSlicingSettings state;
    int selectedSlice = 0;

    juce::ToggleButton enableSlicingButton {"Enable Slicing"};
    juce::ComboBox modeBox;
    juce::ComboBox sliceSelectBox;
    juce::Slider probabilitySlider;
    juce::Slider repeatCountSlider;
    juce::Slider repeatProbabilitySlider;
    juce::Label modeLabel;
    juce::Label sliceLabel;
    juce::Label playProbabilityLabel;
    juce::Label repeatCountLabel;
    juce::Label repeatProbabilityLabel;
    juce::Label helpLabel;
    juce::TextButton autoDetectButton {"Auto Detect Beats"};
    juce::TextButton addSliceButton {"+ Slice"};
    juce::TextButton removeSliceButton {"- Slice"};
    juce::TextButton closeButton {"Close"};

    SliceWaveformView waveformView;

    SlicingChanged slicingChanged;
    std::function<void()> closeRequested;
    bool suppressCallbacks = false;
};
