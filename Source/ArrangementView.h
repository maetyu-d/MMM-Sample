#pragma once

#include <JuceHeader.h>
#include "Track.h"

class ArrangementView : public juce::Component
{
public:
    ArrangementView();

    struct ClipRef
    {
        int trackIndex = -1;
        int clipIndex = -1;
    };

    struct MarkerRef
    {
        int trackIndex = -1;
        int markerIndex = -1;
    };

    enum class SelectionType
    {
        none,
        track,
        clip,
        marker
    };

    using ClipSelected = std::function<void(int trackIndex, int clipIndex)>;
    using TrackSelected = std::function<void(int trackIndex)>;
    using ClipMoved = std::function<void(int sourceTrackIndex, int clipIndex, int targetTrackIndex, int64 newStartSample)>;
    using ClipSplitRequested = std::function<void(int trackIndex, int clipIndex, int64 splitSample)>;
    using ClipTrimRequested = std::function<void(int trackIndex, int clipIndex, int64 newStartSample, int64 newLengthSamples, float newSourceStartNorm, float newSourceEndNorm)>;
    using ClipDeleteRequested = std::function<void(int trackIndex, int clipIndex)>;
    using ClipGlueRequested = std::function<void(const juce::Array<ClipRef>& clips)>;
    using ClipGainEdited = std::function<void(int trackIndex, int clipIndex, float gain)>;
    using ClipFadeEdited = std::function<void(int trackIndex, int clipIndex, float fadeInNorm, float fadeOutNorm)>;
    using ClipFitEdited = std::function<void(int trackIndex, int clipIndex, int fitLengthUnits, bool fitToSnapDivision)>;
    using TrackNameEdited = std::function<void(int trackIndex, const juce::String& name)>;
    using TrackTempoEdited = std::function<void(int trackIndex, double tempoBpm)>;
    using TrackTimeSigEdited = std::function<void(int trackIndex, int numerator, int denominator)>;
    using TrackZoomEdited = std::function<void(int trackIndex, double zoomValue)>;
    using TrackLevelEdited = std::function<void(int trackIndex, double value)>;
    using TrackAutomationEdited = std::function<void(int trackIndex, bool isVolumeLane, const juce::Array<TrackAutomationPoint>& points)>;
    using TrackLoopMarkerEdited = std::function<void(int trackIndex, int markerIndex, int64 samplePosition, bool enabled)>;
    using TrackDeleteRequested = std::function<void(int trackIndex)>;
    using UndoRequested = std::function<void()>;
    using CopyRequested = std::function<void()>;
    using PasteRequested = std::function<void()>;
    using PlayheadDragged = std::function<void(int64 samplePosition)>;
    using CanDragPlayhead = std::function<bool()>;
    using LayoutChanged = std::function<void()>;

    void setTracks(const juce::Array<Track>* tracksIn, double sampleRateIn);
    void setZoomSecondsPerPixel(double secondsPerPixelIn);
    void onClipSelected(ClipSelected cb) { clipSelected = std::move(cb); }
    void onTrackSelected(TrackSelected cb) { trackSelected = std::move(cb); }
    void onClipMoved(ClipMoved cb) { clipMoved = std::move(cb); }
    void onClipSplitRequested(ClipSplitRequested cb) { clipSplitRequested = std::move(cb); }
    void onClipTrimRequested(ClipTrimRequested cb) { clipTrimRequested = std::move(cb); }
    void onClipDeleteRequested(ClipDeleteRequested cb) { clipDeleteRequested = std::move(cb); }
    void onClipGlueRequested(ClipGlueRequested cb) { clipGlueRequested = std::move(cb); }
    void onClipGainEdited(ClipGainEdited cb) { clipGainEdited = std::move(cb); }
    void onClipFadeEdited(ClipFadeEdited cb) { clipFadeEdited = std::move(cb); }
    void onClipFitEdited(ClipFitEdited cb) { clipFitEdited = std::move(cb); }
    void onTrackNameEdited(TrackNameEdited cb) { trackNameEdited = std::move(cb); }
    void onTrackTempoEdited(TrackTempoEdited cb) { trackTempoEdited = std::move(cb); }
    void onTrackTimeSigEdited(TrackTimeSigEdited cb) { trackTimeSigEdited = std::move(cb); }
    void onTrackZoomXEdited(TrackZoomEdited cb) { trackZoomXEdited = std::move(cb); }
    void onTrackZoomYEdited(TrackZoomEdited cb) { trackZoomYEdited = std::move(cb); }
    void onTrackVolumeEdited(TrackLevelEdited cb) { trackVolumeEdited = std::move(cb); }
    void onTrackPanEdited(TrackLevelEdited cb) { trackPanEdited = std::move(cb); }
    void onTrackAutomationEdited(TrackAutomationEdited cb) { trackAutomationEdited = std::move(cb); }
    void onTrackLoopMarkerEdited(TrackLoopMarkerEdited cb) { trackLoopMarkerEdited = std::move(cb); }
    void onTrackDeleteRequested(TrackDeleteRequested cb) { trackDeleteRequested = std::move(cb); }
    void onUndoRequested(UndoRequested cb) { undoRequested = std::move(cb); }
    void onCopyRequested(CopyRequested cb) { copyRequested = std::move(cb); }
    void onPasteRequested(PasteRequested cb) { pasteRequested = std::move(cb); }
    void onPlayheadDragged(PlayheadDragged cb) { playheadDragged = std::move(cb); }
    void onCanDragPlayhead(CanDragPlayhead cb) { canDragPlayhead = std::move(cb); }
    void onLayoutChanged(LayoutChanged cb) { layoutChanged = std::move(cb); }
    enum class EditTool
    {
        select,
        scissors,
        eraser,
        glue
    };
    void setEditTool(EditTool toolIn) { editTool = toolIn; repaint(); }
    EditTool getEditTool() const { return editTool; }
    void setSelectedClip(int trackIndexIn, int clipIndexIn);
    void setPlayheadSample(int64 playheadIn);
    void refreshTrackControls();
    int getRequiredContentHeight() const;
    SelectionType getSelectionType() const { return selectionType; }
    const juce::Array<int>& getSelectedTracks() const { return selectedTracks; }
    const juce::Array<ClipRef>& getSelectedClips() const { return selectedClips; }
    const juce::Array<MarkerRef>& getSelectedMarkers() const { return selectedMarkers; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    enum class EditField
    {
        none,
        name,
        tempo,
        timeSigNum,
        timeSigDen
    };

    int yToTrackIndex(float y) const;
    int getTrackTop(int trackIndex) const;
    int getTrackHeight(int trackIndex) const;
    int getTimelineStartX() const;
    int64 xToSampleForTrack(int trackIndex, float x) const;
    int sampleToXForTrack(int trackIndex, int64 samplePosition) const;
    juce::Rectangle<int> getTrackHeaderBounds(int trackIndex) const;
    juce::Rectangle<int> getNameFieldBounds(int trackIndex) const;
    juce::Rectangle<int> getTempoFieldBounds(int trackIndex) const;
    juce::Rectangle<int> getTimeSigNumFieldBounds(int trackIndex) const;
    juce::Rectangle<int> getTimeSigDenFieldBounds(int trackIndex) const;
    juce::Rectangle<int> getAutomationToggleBounds(int trackIndex) const;
    juce::Rectangle<int> getClipLaneBounds(int trackIndex) const;
    juce::Rectangle<int> getAutomationLaneBounds(int trackIndex, bool volumeLane) const;
    int64 getClipPlaybackLengthSamples(const Track& track, const TrackClip& clip) const;
    juce::Rectangle<int> getClipRect(int trackIndex, int clipIndex) const;
    juce::Rectangle<int> getClipGainHandleBounds(const juce::Rectangle<int>& clipRect) const;
    juce::Rectangle<int> getClipFadeInHandleBounds(const juce::Rectangle<int>& clipRect) const;
    juce::Rectangle<int> getClipFadeOutHandleBounds(const juce::Rectangle<int>& clipRect) const;
    juce::Rectangle<int> getClipFitUnitsBounds(const juce::Rectangle<int>& clipRect) const;
    juce::Rectangle<int> getClipFitModeBounds(const juce::Rectangle<int>& clipRect) const;
    juce::Rectangle<int> getLeftNameFieldBounds(int trackIndex) const;
    juce::Rectangle<int> getLeftTempoFieldBounds(int trackIndex) const;
    juce::Rectangle<int> getLeftTimeSigNumFieldBounds(int trackIndex) const;
    juce::Rectangle<int> getLeftTimeSigDenFieldBounds(int trackIndex) const;
    juce::Rectangle<int> getMixPadBounds(int trackIndex) const;
    juce::Rectangle<int> getZoomPadBounds(int trackIndex) const;
    int getDividerX(int dividerIndex) const;
    bool hitColumnDivider(int x, int y, int& dividerIndexOut) const;
    void setColumnWidthsFromDividerDrag(int dividerIndex, int mouseX);
    bool hitTrackHeightHandle(int x, int y, int& trackIndexOut) const;
    bool hitLoopMarker(int trackIndex, int mouseX, int& markerIndexOut) const;
    bool hitAutomationPoint(int trackIndex, bool volumeLane, juce::Point<float> pos, int& pointIndexOut) const;
    double yToAutomationValue(bool volumeLane, int y, const juce::Rectangle<int>& laneBounds) const;
    int automationValueToY(bool volumeLane, double value, const juce::Rectangle<int>& laneBounds) const;
    void startInlineEdit(int trackIndex, EditField field, const juce::Rectangle<int>& bounds, const juce::String& value);
    void finishInlineEdit();
    void setSingleTrackSelection(int trackIndex);
    void setSingleClipSelection(int trackIndex, int clipIndex);
    void setSingleMarkerSelection(int trackIndex, int markerIndex);
    void toggleTrackSelection(int trackIndex);
    void toggleClipSelection(int trackIndex, int clipIndex);
    void toggleMarkerSelection(int trackIndex, int markerIndex);
    bool isTrackSelected(int trackIndex) const;
    bool isClipSelected(int trackIndex, int clipIndex) const;
    bool isMarkerSelected(int trackIndex, int markerIndex) const;
    int findClipSelectionIndex(int trackIndex, int clipIndex) const;
    int findMarkerSelectionIndex(int trackIndex, int markerIndex) const;
    void ensureZoomSliders();
    void ensureAutomationExpandedState();
    void layoutZoomSliders();

    const juce::Array<Track>* tracks = nullptr;
    double sampleRate = 44100.0;
    double secondsPerPixel = 0.01; // 100 px per second

    int trackHeaderWidth = 340;
    int trackControlsWidth = 190;
    int minTrackControlsWidth = 130;
    int minTrackHeaderWidth = 220;
    int minTimelineWidth = 360;
    int trackHeight = 80;
    int headerHeight = 24;
    int timelineStartGap = 16;

    int selectedTrack = -1;
    int selectedClip = -1;
    ClipSelected clipSelected;
    TrackSelected trackSelected;
    ClipMoved clipMoved;
    ClipSplitRequested clipSplitRequested;
    ClipTrimRequested clipTrimRequested;
    ClipDeleteRequested clipDeleteRequested;
    ClipGlueRequested clipGlueRequested;
    ClipGainEdited clipGainEdited;
    ClipFadeEdited clipFadeEdited;
    ClipFitEdited clipFitEdited;
    TrackNameEdited trackNameEdited;
    TrackTempoEdited trackTempoEdited;
    TrackTimeSigEdited trackTimeSigEdited;
    TrackZoomEdited trackZoomXEdited;
    TrackZoomEdited trackZoomYEdited;
    TrackLevelEdited trackVolumeEdited;
    TrackLevelEdited trackPanEdited;
    TrackAutomationEdited trackAutomationEdited;
    TrackLoopMarkerEdited trackLoopMarkerEdited;
    TrackDeleteRequested trackDeleteRequested;
    UndoRequested undoRequested;
    CopyRequested copyRequested;
    PasteRequested pasteRequested;
    PlayheadDragged playheadDragged;
    CanDragPlayhead canDragPlayhead;
    LayoutChanged layoutChanged;
    SelectionType selectionType = SelectionType::none;
    juce::Array<int> selectedTracks;
    juce::Array<ClipRef> selectedClips;
    juce::Array<MarkerRef> selectedMarkers;

    bool draggingClip = false;
    bool draggingClipTrimStart = false;
    bool draggingClipTrimEnd = false;
    bool draggingPlayhead = false;
    int playheadDragTrack = -1;
    bool draggingClipGain = false;
    bool draggingClipFadeIn = false;
    bool draggingClipFadeOut = false;
    int editClipTrack = -1;
    int editClipIndex = -1;
    int dragSourceTrack = -1;
    int dragClipIndex = -1;
    int dragTargetTrack = -1;
    int64 dragStartSample = 0;
    int64 dragOriginalStartSample = 0;
    int64 dragLengthSamples = 0;
    int64 dragPointerOffsetSamples = 0;
    bool clipDragMoved = false;
    int64 trimOriginalStartSample = 0;
    int64 trimOriginalLengthSamples = 0;
    float trimOriginalSourceStartNorm = 0.0f;
    float trimOriginalSourceEndNorm = 1.0f;
    int lastMouseX = -1;
    int lastMouseY = -1;
    EditTool editTool = EditTool::select;
    int64 playheadSample = 0;
    bool draggingLoopMarker = false;
    int selectedLoopMarkerTrack = -1;
    int selectedLoopMarkerIndex = -1;
    bool draggingColumnDivider = false;
    int draggingDividerIndex = -1;
    bool draggingTrackHeight = false;
    int draggingTrackHeightTrack = -1;
    bool draggingMixPad = false;
    int draggingMixPadTrack = -1;
    bool draggingZoomPad = false;
    int draggingZoomPadTrack = -1;
    bool draggingAutomationPoint = false;
    int selectedAutomationTrack = -1;
    int selectedAutomationPointIndex = -1;
    bool selectedAutomationVolumeLane = true;

    juce::TextEditor inlineEditor;
    juce::Array<bool> automationExpanded;
    EditField editingField = EditField::none;
    int editingTrack = -1;
    bool suppressInlineCommit = false;
    bool suppressZoomSliderCallbacks = false;
};
