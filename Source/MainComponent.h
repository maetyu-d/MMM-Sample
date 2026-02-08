#pragma once

#include <JuceHeader.h>
#include "AudioEngine.h"
#include "ArrangementView.h"
#include "WarpCurveEditor.h"
#include "WarpPanel.h"
#include "BeatSlicerComponent.h"

class MainComponent : public juce::AudioAppComponent,
                      public juce::Button::Listener,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    void styleToolbarButton(juce::TextButton& button, juce::Colour baseColour, bool isPrimary);
    void updateArrangementViewportBounds(bool preserveScroll = true);
    void addSampleToTrack(const juce::File& file);
    void applyWarpToSelectedClip(const WarpCurve& curve, const juce::String& label);
    void selectClip(int trackIndex, int clipIndex);
    void selectTrack(int trackIndex);
    void moveClip(int sourceTrackIndex, int clipIndex, int targetTrackIndex, int64 newStartSample);
    void splitClip(int trackIndex, int clipIndex, int64 splitSample);
    void trimClip(int trackIndex, int clipIndex, int64 newStartSample, int64 newLengthSamples, float newSourceStartNorm, float newSourceEndNorm);
    void deleteClip(int trackIndex, int clipIndex);
    void glueClips(const juce::Array<ArrangementView::ClipRef>& clipRefs);
    void setActiveEditTool(ArrangementView::EditTool tool);
    void editTrackName(int trackIndex, const juce::String& name);
    void editTrackTempo(int trackIndex, double tempoBpm);
    void editTrackTimeSignature(int trackIndex, int numerator, int denominator);
    void editTrackLoopMarker(int trackIndex, int markerIndex, int64 samplePosition, bool enabled);
    void editTrackZoomX(int trackIndex, double zoomValue);
    void editTrackZoomY(int trackIndex, double zoomValue);
    void editTrackVolume(int trackIndex, double volumeValue);
    void editTrackPan(int trackIndex, double panValue);
    void editClipGain(int trackIndex, int clipIndex, float gain);
    void editClipFade(int trackIndex, int clipIndex, float fadeInNorm, float fadeOutNorm);
    void editClipFit(int trackIndex, int clipIndex, int fitLengthUnits, bool fitToSnapDivision);
    void editTrackAutomationPoints(int trackIndex, bool isVolumeLane, const juce::Array<TrackAutomationPoint>& points);
    void refreshClipFitLengthsForTrack(int trackIndex);
    void deleteTrack(int trackIndex);
    void updateTrackControlsFromSelection();
    void applyTrackTimingFromControls();
    void showLoadDialog();
    void showSaveProjectDialog();
    void showLoadProjectDialog();
    void showAudioSettingsDialog();
    juce::var createProjectStateVar();
    bool loadProjectFromVar(const juce::var& parsed, bool preserveScroll);
    void pushUndoState();
    void performUndo();
    void performCopy();
    void performPaste();
    juce::File getAppSettingsFile() const;
    void loadAppSettings();
    void saveAppSettings() const;
    void showExportDialog();
    bool saveProjectToFile(const juce::File& file);
    bool loadProjectFromFile(const juce::File& file);
    bool writeWavFile(const juce::File& file, const juce::AudioBuffer<float>& buffer) const;
    void normalizeBuffer(juce::AudioBuffer<float>& buffer) const;
    int64 getTrackEndSamples(const Track& track) const;
    void renderTrackToBuffer(Track& track, juce::AudioBuffer<float>& buffer) const;
    void exportOneTrack(const juce::File& outputFile, bool normalize);
    void exportStems(const juce::File& outputDirectory, bool normalize);
    void exportAllTracksMix(const juce::File& outputFile, bool normalize);
    void openBeatSlicerForSelection();
    void applySlicingToSelectedClip(const BeatSlicingSettings& slicing);
    void applyFitSettingsToSelectedClip(int fitLengthUnits, bool fitToSnapDivision);

    AudioEngine engine;

    juce::TextButton playButton {"Play"};
    juce::TextButton stopButton {"Stop"};
    juce::TextButton scissorsButton {"Scissors"};
    juce::TextButton eraserButton {"Eraser"};
    juce::TextButton glueButton {"Glue"};
    juce::TextButton loadButton {"Load WAV"};
    juce::TextButton exportButton {"Export Audio"};
    juce::TextButton projectButton {"Project"};
    juce::TextButton addTrackButton {"Add Track"};
    juce::TextButton settingsButton {"Settings"};
    juce::ToggleButton snapToggle {"Snap"};
    juce::ComboBox snapDivisionBox;
    juce::Label transportSectionLabel;
    juce::Label mediaSectionLabel;
    juce::Label projectSectionLabel;
    juce::Label trackSectionLabel;
    juce::Slider tempoSlider;
    juce::ComboBox timeSigNumBox;
    juce::ComboBox timeSigDenBox;
    juce::Label tempoLabel;
    juce::Label timeSigLabel;
    juce::Viewport arrangementViewport;
    ArrangementView arrangementView;
    WarpCurveEditor curveEditor;
    WarpPanel warpPanel;
    BeatSlicerComponent beatSlicer;
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::FileChooser> projectFileChooser;
    std::unique_ptr<juce::FileChooser> exportFileChooser;

    double sampleRate = 44100.0;
    int selectedTrackIndex = -1;
    int selectedClipIndex = -1;
    bool suppressTrackControlCallbacks = false;
    int exportBitDepth = 24;
    std::unique_ptr<juce::XmlElement> pendingAudioDeviceState;
    juce::Array<juce::String> undoStack;
    bool isApplyingUndo = false;

    enum class ClipboardType
    {
        none,
        track,
        clip,
        marker
    };

    struct CopiedClip
    {
        int sourceTrackIndex = -1;
        int64 startSample = 0;
        TrackClip clip;
    };

    struct CopiedMarker
    {
        int sourceTrackIndex = -1;
        int markerIndex = -1;
        int64 samplePosition = 0;
    };

    ClipboardType clipboardType = ClipboardType::none;
    juce::Array<Track> copiedTracks;
    juce::Array<CopiedClip> copiedClips;
    juce::Array<CopiedMarker> copiedMarkers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
