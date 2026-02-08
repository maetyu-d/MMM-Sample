#pragma once

#include <JuceHeader.h>
#include "WarpCurve.h"

class WarpPanel : public juce::Component,
                  public juce::Button::Listener,
                  private juce::ListBoxModel
{
public:
    using PresetSelected = std::function<void(const WarpCurve&)>;
    using CurveEdited = std::function<void(const WarpCurve&)>;
    using FitSettingsEdited = std::function<void(int fitLengthUnits, bool fitToSnapDivision)>;

    WarpPanel();

    void setCurve(const WarpCurve& curveIn);
    void setFitSettings(int fitLengthUnits, bool fitToSnapDivision);
    void onPresetSelected(PresetSelected cb) { presetSelected = std::move(cb); }
    void onCurveEdited(CurveEdited cb) { curveEdited = std::move(cb); }
    void onFitSettingsEdited(FitSettingsEdited cb) { fitSettingsEdited = std::move(cb); }

    void paint(juce::Graphics& g) override;
    void resized() override;

    void buttonClicked(juce::Button* button) override;

private:
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;
    void updatePoint(int index, WarpCurve::Point p);
    void deletePoint(int index);
    void notifyCurveChanged();

    juce::TextButton linearButton {"Linear"};
    juce::TextButton slowFastButton {"Slow->Fast"};
    juce::TextButton fastSlowButton {"Fast->Slow"};
    juce::ToggleButton smoothToggle {"Smooth rate transitions"};
    juce::Label fitUnitsLabel;
    juce::ComboBox fitUnitsBox;
    juce::Label fitModeLabel;
    juce::ComboBox fitModeBox;

    juce::ListBox pointList {"Points", this};
    juce::Array<WarpCurve::Point> points;
    bool smoothRateChanges = false;
    bool suppressCallbacks = false;

    PresetSelected presetSelected;
    CurveEdited curveEdited;
    FitSettingsEdited fitSettingsEdited;
};
