#include "WarpPanel.h"

namespace
{
class PointRowComponent : public juce::Component
{
public:
    using Changed = std::function<void(int, WarpCurve::Point)>;
    using Deleted = std::function<void(int)>;

    PointRowComponent()
    {
        addAndMakeVisible(tEditor);
        addAndMakeVisible(vEditor);
        addAndMakeVisible(deleteButton);

        tEditor.setInputRestrictions(0, "0123456789.");
        vEditor.setInputRestrictions(0, "0123456789.");

        tEditor.onReturnKey = [this]() { commitEdit(); };
        vEditor.onReturnKey = [this]() { commitEdit(); };
        tEditor.onFocusLost = [this]() { commitEdit(); };
        vEditor.onFocusLost = [this]() { commitEdit(); };

        deleteButton.onClick = [this]()
        {
            if (onDeleted)
                onDeleted(index);
        };
    }

    void configure(int indexIn, const WarpCurve::Point& p, bool canDeleteIn)
    {
        index = indexIn;
        canDelete = canDeleteIn;
        current = p;
        tEditor.setText(juce::String(p.t, 4), juce::dontSendNotification);
        vEditor.setText(juce::String(p.v, 4), juce::dontSendNotification);
        deleteButton.setEnabled(canDelete);
        deleteButton.setAlpha(canDelete ? 1.0f : 0.4f);
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawLine(0.0f, (float)getHeight() - 1.0f, (float)getWidth(), (float)getHeight() - 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(4);
        auto tArea = area.removeFromLeft(70);
        auto vArea = area.removeFromLeft(70);
        area.removeFromLeft(6);

        tEditor.setBounds(tArea);
        vEditor.setBounds(vArea);
        deleteButton.setBounds(area.removeFromLeft(50));
    }

    Changed onChanged;
    Deleted onDeleted;

private:
    void commitEdit()
    {
        if (!onChanged)
            return;

        WarpCurve::Point p = current;
        p.t = tEditor.getText().getDoubleValue();
        p.v = vEditor.getText().getDoubleValue();
        onChanged(index, p);
    }

    int index = -1;
    bool canDelete = false;
    WarpCurve::Point current;
    juce::TextEditor tEditor;
    juce::TextEditor vEditor;
    juce::TextButton deleteButton {"Del"};
};
} // namespace

WarpPanel::WarpPanel()
{
    addAndMakeVisible(linearButton);
    addAndMakeVisible(slowFastButton);
    addAndMakeVisible(fastSlowButton);
    addAndMakeVisible(smoothToggle);
    addAndMakeVisible(fitUnitsLabel);
    addAndMakeVisible(fitUnitsBox);
    addAndMakeVisible(fitModeLabel);
    addAndMakeVisible(fitModeBox);
    addAndMakeVisible(pointList);

    linearButton.addListener(this);
    slowFastButton.addListener(this);
    fastSlowButton.addListener(this);
    smoothToggle.onClick = [this]()
    {
        if (suppressCallbacks)
            return;
        smoothRateChanges = smoothToggle.getToggleState();
        notifyCurveChanged();
    };
    fitUnitsLabel.setText("Units", juce::dontSendNotification);
    fitUnitsLabel.setJustificationType(juce::Justification::centredLeft);
    fitUnitsLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
    fitUnitsBox.addItem("1", 1);
    fitUnitsBox.addItem("2", 2);
    fitUnitsBox.addItem("4", 4);
    fitUnitsBox.addItem("8", 8);
    fitUnitsBox.setSelectedId(1, juce::dontSendNotification);
    fitUnitsBox.onChange = [this]()
    {
        if (suppressCallbacks || !fitSettingsEdited)
            return;
        fitSettingsEdited(juce::jmax(1, fitUnitsBox.getSelectedId()), fitModeBox.getSelectedId() == 2);
    };
    fitModeLabel.setText("Fit", juce::dontSendNotification);
    fitModeLabel.setJustificationType(juce::Justification::centredLeft);
    fitModeLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
    fitModeBox.addItem("Bars", 1);
    fitModeBox.addItem("Snap", 2);
    fitModeBox.setSelectedId(1, juce::dontSendNotification);
    fitModeBox.onChange = [this]()
    {
        if (suppressCallbacks || !fitSettingsEdited)
            return;
        fitSettingsEdited(juce::jmax(1, fitUnitsBox.getSelectedId()), fitModeBox.getSelectedId() == 2);
    };
    fitUnitsBox.setTooltip("Set clip target length in bars or snap units.");
    fitModeBox.setTooltip("Choose whether units are bars or current track snap division.");
    fitUnitsBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(38, 44, 56));
    fitUnitsBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.2f));
    fitUnitsBox.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.9f));
    fitModeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(38, 44, 56));
    fitModeBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.2f));
    fitModeBox.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.9f));

    pointList.setRowHeight(24);
    pointList.setOutlineThickness(1);
    pointList.setColour(juce::ListBox::outlineColourId, juce::Colour(50, 55, 65));
}

void WarpPanel::setCurve(const WarpCurve& curveIn)
{
    const juce::ScopedValueSetter<bool> syncGuard(suppressCallbacks, true);
    points = curveIn.getPoints();
    smoothRateChanges = curveIn.getSmoothRateChanges();
    smoothToggle.setToggleState(smoothRateChanges, juce::dontSendNotification);
    pointList.updateContent();
    pointList.repaint();
    repaint();
}

void WarpPanel::setFitSettings(int fitLengthUnits, bool fitToSnapDivision)
{
    const juce::ScopedValueSetter<bool> syncGuard(suppressCallbacks, true);
    int unitsId = 1;
    if (fitLengthUnits >= 8) unitsId = 8;
    else if (fitLengthUnits >= 4) unitsId = 4;
    else if (fitLengthUnits >= 2) unitsId = 2;
    fitUnitsBox.setSelectedId(unitsId, juce::dontSendNotification);
    fitModeBox.setSelectedId(fitToSnapDivision ? 2 : 1, juce::dontSendNotification);
}

void WarpPanel::paint(juce::Graphics& g)
{
    constexpr int outerPad = 10;
    constexpr int titleH = 20;
    constexpr int fitRowH = 28;
    constexpr int sectionGap = 12;
    constexpr int presetsTitleGap = 6;

    g.fillAll(juce::Colour(20, 22, 26));

    const int x = outerPad;
    const int w = getWidth() - outerPad * 2;
    const int clipTitleY = 6;
    const int fitRowY = clipTitleY + titleH + 6;
    const int warpTitleY = fitRowY + fitRowH + sectionGap;
    const juce::Rectangle<float> fitCard((float)x, (float)(fitRowY - 1), (float)w, (float)(fitRowH + 2));

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawText("Clip Length", x, clipTitleY, w, titleH, juce::Justification::centredLeft);
    g.setColour(juce::Colour(30, 35, 44).withAlpha(0.88f));
    g.fillRoundedRectangle(fitCard, 5.0f);
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawRoundedRectangle(fitCard, 5.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.34f));
    g.drawText("Warp Presets & Points", x, warpTitleY, w, 16, juce::Justification::centredLeft, false);

    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.drawLine((float)x, (float)(fitRowY + fitRowH + sectionGap - presetsTitleGap),
               (float)(x + w), (float)(fitRowY + fitRowH + sectionGap - presetsTitleGap), 1.0f);
}

void WarpPanel::resized()
{
    constexpr int outerPad = 10;
    constexpr int titleH = 20;
    constexpr int fitRowH = 28;
    constexpr int sectionGap = 12;
    constexpr int presetsTitleH = 16;
    constexpr int presetsRowH = 28;

    auto area = getLocalBounds().reduced(outerPad);
    area.removeFromTop(titleH + 6); // "Clip Length" title

    auto fitRow = area.removeFromTop(fitRowH);
    fitUnitsLabel.setBounds(fitRow.removeFromLeft(40));
    fitUnitsBox.setBounds(fitRow.removeFromLeft(78));
    fitRow.removeFromLeft(12);
    fitModeLabel.setBounds(fitRow.removeFromLeft(28));
    fitModeBox.setBounds(fitRow.removeFromLeft(90));

    area.removeFromTop(sectionGap + presetsTitleH); // separator + "Warp Presets & Points" title

    auto row = area.removeFromTop(presetsRowH);
    linearButton.setBounds(row.removeFromLeft(76));
    row.removeFromLeft(8);
    slowFastButton.setBounds(row.removeFromLeft(96));
    row.removeFromLeft(8);
    fastSlowButton.setBounds(row.removeFromLeft(96));
    row.removeFromLeft(12);
    smoothToggle.setBounds(row.removeFromLeft(176));

    area.removeFromTop(10);
    pointList.setBounds(area.reduced(0, 0));
}

void WarpPanel::buttonClicked(juce::Button* button)
{
    if (!presetSelected)
        return;

    auto applyPreset = [this](WarpCurve curve)
    {
        curve.setRelativeMode(true);
        curve.setSmoothRateChanges(smoothRateChanges);
        presetSelected(curve);
    };

    if (button == &linearButton)
        applyPreset(WarpCurve::linear());
    else if (button == &slowFastButton)
        applyPreset(WarpCurve::slowToFast());
    else if (button == &fastSlowButton)
        applyPreset(WarpCurve::fastToSlow());
}

int WarpPanel::getNumRows()
{
    return points.size();
}

void WarpPanel::paintListBoxItem(int, juce::Graphics&, int, int, bool)
{
}

juce::Component* WarpPanel::refreshComponentForRow(int rowNumber, bool, juce::Component* existingComponentToUpdate)
{
    if (rowNumber < 0 || rowNumber >= points.size())
        return nullptr;

    auto* row = dynamic_cast<PointRowComponent*>(existingComponentToUpdate);
    if (row == nullptr)
    {
        row = new PointRowComponent();
        row->onChanged = [this](int index, WarpCurve::Point p)
        {
            updatePoint(index, p);
        };
        row->onDeleted = [this](int index)
        {
            deletePoint(index);
        };
    }

    const bool canDelete = (rowNumber > 0 && rowNumber < points.size() - 1);
    row->configure(rowNumber, points.getReference(rowNumber), canDelete);
    return row;
}

void WarpPanel::updatePoint(int index, WarpCurve::Point p)
{
    if (suppressCallbacks)
        return;

    if (index < 0 || index >= points.size())
        return;

    if (index == 0)
    {
        p.t = 0.0;
        p.v = juce::jlimit(0.0, 1.0, p.v);
    }
    else if (index == points.size() - 1)
    {
        p.t = 1.0;
        p.v = juce::jlimit(0.0, 1.0, p.v);
    }
    else
    {
        const auto& left = points.getReference(index - 1);
        const auto& right = points.getReference(index + 1);
        p.t = juce::jlimit(left.t + 0.001, right.t - 0.001, p.t);
        p.v = juce::jlimit(0.0, 1.0, p.v);
    }

    points.set(index, p);
    notifyCurveChanged();
    pointList.updateContent();
    pointList.repaintRow(index);
}

void WarpPanel::deletePoint(int index)
{
    if (suppressCallbacks)
        return;

    if (index <= 0 || index >= points.size() - 1)
        return;
    points.remove(index);
    notifyCurveChanged();
    pointList.updateContent();
    pointList.repaint();
}

void WarpPanel::notifyCurveChanged()
{
    if (suppressCallbacks || !curveEdited)
        return;

    WarpCurve curve;
    curve.setPoints(points);
    curve.setRelativeMode(true);
    curve.setSmoothRateChanges(smoothRateChanges);
    curveEdited(curve);
}
