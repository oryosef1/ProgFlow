# ProgFlow UI Redesign Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Transform the UI from tiny-knobs-in-giant-boxes to Ableton-style dense, clean, professional interface.

**Architecture:** Update core components (RotaryKnob, LookAndFeel), create shared SynthEditorBase class, then refactor all synth editors and panels to use the new system with thin divider lines instead of boxes.

**Tech Stack:** JUCE framework, C++17

---

## Phase 1: Core Components

### Task 1: Update RotaryKnob with fixed 48px sizing

**Files:**
- Modify: `Source/UI/Components/RotaryKnob.h`
- Modify: `Source/UI/Components/RotaryKnob.cpp`

**Step 1: Update RotaryKnob.h - add size constants**

In `Source/UI/Components/RotaryKnob.h`, add constants after the class declaration begins:

```cpp
class RotaryKnob : public juce::Component
{
public:
    // Fixed sizes for consistent UI
    static constexpr int KNOB_DIAMETER = 48;
    static constexpr int LABEL_HEIGHT = 16;
    static constexpr int LABEL_GAP = 4;
    static constexpr int TOTAL_HEIGHT = KNOB_DIAMETER + LABEL_GAP + LABEL_HEIGHT; // 68px
```

**Step 2: Update RotaryKnob.cpp - enforce fixed sizing in resized()**

Find the `resized()` method and replace it with:

```cpp
void RotaryKnob::resized()
{
    auto bounds = getLocalBounds();

    // Label at bottom
    auto labelArea = bounds.removeFromBottom(LABEL_HEIGHT);
    label.setBounds(labelArea);

    // Gap
    bounds.removeFromBottom(LABEL_GAP);

    // Knob centered in remaining space
    auto knobArea = bounds.withSizeKeepingCentre(KNOB_DIAMETER, KNOB_DIAMETER);
    slider.setBounds(knobArea);
}
```

**Step 3: Build and verify**

Run: `cd /Users/orperelman/Downloads/ProgFlow-JUCE/build && cmake --build . --config Release 2>&1 | tail -20`
Expected: Build succeeds

**Step 4: Commit**

```bash
cd /Users/orperelman/Downloads/ProgFlow-JUCE
git add Source/UI/Components/RotaryKnob.h Source/UI/Components/RotaryKnob.cpp
git commit -m "feat(ui): add fixed 48px sizing constants to RotaryKnob"
```

---

### Task 2: Update LookAndFeel colors and spacing

**Files:**
- Modify: `Source/UI/LookAndFeel.h`
- Modify: `Source/UI/LookAndFeel.cpp`

**Step 1: Update spacing constants in LookAndFeel.h**

Replace the `ProgFlowSpacing` namespace with:

```cpp
namespace ProgFlowSpacing
{
    static constexpr int XS = 4;
    static constexpr int SM = 8;
    static constexpr int MD = 12;
    static constexpr int LG = 16;
    static constexpr int XL = 24;

    // Component sizes
    static constexpr int KNOB_SIZE = 48;
    static constexpr int KNOB_WITH_LABEL = 68;
    static constexpr int SECTION_HEADER_HEIGHT = 16;
    static constexpr int COMBO_HEIGHT = 24;
    static constexpr int DIVIDER_WIDTH = 1;

    // Deprecated - remove usage
    static constexpr int KNOB_MIN_SIZE = 48;
    static constexpr int KNOB_PREFERRED = 48;
    static constexpr int SECTION_CORNER_RADIUS = 6;
    static constexpr int BUTTON_CORNER_RADIUS = 4;
}
```

**Step 2: Add divider color to ColorScheme in LookAndFeel.h**

In the `ColorScheme` struct, add after `surfaceBg`:

```cpp
    juce::Colour dividerLine;    // Thin section dividers
```

**Step 3: Update dark scheme colors in LookAndFeel.cpp**

In the `ThemeManager` constructor, update the dark scheme:

```cpp
    // Dark scheme
    darkScheme.bgPrimary = juce::Colour(0xff1a1a1a);
    darkScheme.bgSecondary = juce::Colour(0xff252525);
    darkScheme.bgTertiary = juce::Colour(0xff2a2a2a);
    darkScheme.bgHover = juce::Colour(0xff333333);
    darkScheme.sectionBg = juce::Colour(0xff1e1e1e);
    darkScheme.surfaceBg = juce::Colour(0xff2d2d2d);
    darkScheme.dividerLine = juce::Colour(0xff2a2a2a);
```

**Step 4: Add dividerLine accessor to ProgFlowColours namespace in LookAndFeel.h**

After the other inline functions:

```cpp
    inline juce::Colour dividerLine() { return ThemeManager::getInstance().getColors().dividerLine; }
```

**Step 5: Build and verify**

Run: `cd /Users/orperelman/Downloads/ProgFlow-JUCE/build && cmake --build . --config Release 2>&1 | tail -20`
Expected: Build succeeds

**Step 6: Commit**

```bash
cd /Users/orperelman/Downloads/ProgFlow-JUCE
git add Source/UI/LookAndFeel.h Source/UI/LookAndFeel.cpp
git commit -m "feat(ui): update spacing constants and add divider colors"
```

---

### Task 3: Create SynthEditorBase class

**Files:**
- Create: `Source/UI/Synths/SynthEditorBase.h`
- Create: `Source/UI/Synths/SynthEditorBase.cpp`

**Step 1: Create SynthEditorBase.h**

```cpp
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../LookAndFeel.h"
#include "../Components/RotaryKnob.h"

/**
 * SynthEditorBase - Base class for all synth editors
 *
 * Provides:
 * - Common header row (preset selector + master volume)
 * - Divider line drawing utilities
 * - Consistent section layout helpers
 */
class SynthEditorBase : public juce::Component,
                        public juce::ComboBox::Listener
{
public:
    SynthEditorBase();
    ~SynthEditorBase() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

protected:
    // Layout constants
    static constexpr int HEADER_HEIGHT = 40;
    static constexpr int SECTION_PADDING = 12;
    static constexpr int KNOB_SIZE = 48;
    static constexpr int KNOB_SPACING = 8;

    // Header components (subclasses set up preset selector)
    juce::Label presetLabel;
    juce::ComboBox presetSelector;
    juce::Label masterLabel;
    RotaryKnob masterVolume;

    // Divider drawing helpers
    void drawVerticalDivider(juce::Graphics& g, int x, int yStart, int yEnd);
    void drawHorizontalDivider(juce::Graphics& g, int xStart, int xEnd, int y);

    // Section layout helpers
    struct Section {
        juce::String name;
        float widthRatio;  // Proportion of total width (0.0 to 1.0)
    };

    // Calculate section bounds given sections and total area
    std::vector<juce::Rectangle<int>> calculateSectionBounds(
        const std::vector<Section>& sections,
        juce::Rectangle<int> area);

    // Get content area (below header)
    juce::Rectangle<int> getContentArea();

    // Create consistent section label
    void createSectionLabel(juce::Label& label, const juce::String& text);

    // Subclasses override these
    virtual void layoutHeader(juce::Rectangle<int> area);
    virtual void layoutContent(juce::Rectangle<int> area) = 0;
    virtual void drawDividers(juce::Graphics& g, juce::Rectangle<int> contentArea);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthEditorBase)
};
```

**Step 2: Create SynthEditorBase.cpp**

```cpp
#include "SynthEditorBase.h"

SynthEditorBase::SynthEditorBase()
{
    // Preset label
    presetLabel.setText("PRESET", juce::dontSendNotification);
    presetLabel.setFont(juce::Font(10.0f));
    presetLabel.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    presetLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(presetLabel);

    // Preset selector
    addAndMakeVisible(presetSelector);

    // Master label
    masterLabel.setText("MASTER", juce::dontSendNotification);
    masterLabel.setFont(juce::Font(10.0f));
    masterLabel.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    masterLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(masterLabel);

    // Master volume
    masterVolume.setLabel("Vol");
    addAndMakeVisible(masterVolume);
}

SynthEditorBase::~SynthEditorBase()
{
}

void SynthEditorBase::paint(juce::Graphics& g)
{
    g.fillAll(ProgFlowColours::bgPrimary());

    // Draw header divider
    drawHorizontalDivider(g, 0, getWidth(), HEADER_HEIGHT);

    // Let subclass draw content dividers
    drawDividers(g, getContentArea());
}

void SynthEditorBase::resized()
{
    auto bounds = getLocalBounds();

    // Header
    auto headerArea = bounds.removeFromTop(HEADER_HEIGHT);
    layoutHeader(headerArea);

    // Content
    layoutContent(bounds);
}

void SynthEditorBase::layoutHeader(juce::Rectangle<int> area)
{
    area = area.reduced(ProgFlowSpacing::MD, ProgFlowSpacing::SM);

    // Preset on left
    presetLabel.setBounds(area.removeFromLeft(50).withHeight(16).withY(area.getCentreY() - 8));
    area.removeFromLeft(4);
    presetSelector.setBounds(area.removeFromLeft(180).withHeight(24).withY(area.getCentreY() - 12));

    // Master volume on right
    auto masterArea = area.removeFromRight(KNOB_SIZE + 20);
    masterLabel.setBounds(masterArea.removeFromTop(14));
    masterVolume.setBounds(masterArea.withSizeKeepingCentre(KNOB_SIZE, KNOB_SIZE + 20));
}

juce::Rectangle<int> SynthEditorBase::getContentArea()
{
    return getLocalBounds().withTrimmedTop(HEADER_HEIGHT);
}

void SynthEditorBase::drawVerticalDivider(juce::Graphics& g, int x, int yStart, int yEnd)
{
    g.setColour(ProgFlowColours::dividerLine());
    g.fillRect(x, yStart, ProgFlowSpacing::DIVIDER_WIDTH, yEnd - yStart);
}

void SynthEditorBase::drawHorizontalDivider(juce::Graphics& g, int xStart, int xEnd, int y)
{
    g.setColour(ProgFlowColours::dividerLine());
    g.fillRect(xStart, y, xEnd - xStart, ProgFlowSpacing::DIVIDER_WIDTH);
}

std::vector<juce::Rectangle<int>> SynthEditorBase::calculateSectionBounds(
    const std::vector<Section>& sections,
    juce::Rectangle<int> area)
{
    std::vector<juce::Rectangle<int>> result;
    int totalWidth = area.getWidth();
    int x = area.getX();

    for (size_t i = 0; i < sections.size(); ++i)
    {
        int sectionWidth = static_cast<int>(totalWidth * sections[i].widthRatio);
        result.push_back(juce::Rectangle<int>(x, area.getY(), sectionWidth, area.getHeight()));
        x += sectionWidth;
    }

    return result;
}

void SynthEditorBase::createSectionLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text.toUpperCase(), juce::dontSendNotification);
    label.setFont(juce::Font(10.0f));
    label.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    label.setJustificationType(juce::Justification::centredLeft);
}

void SynthEditorBase::drawDividers(juce::Graphics& g, juce::Rectangle<int> contentArea)
{
    // Default: no dividers. Subclasses override.
    juce::ignoreUnused(g, contentArea);
}
```

**Step 3: Add to CMakeLists.txt**

Find the `target_sources(ProgFlow PRIVATE` section and add:

```cmake
    Source/UI/Synths/SynthEditorBase.h
    Source/UI/Synths/SynthEditorBase.cpp
```

**Step 4: Build and verify**

Run: `cd /Users/orperelman/Downloads/ProgFlow-JUCE/build && cmake --build . --config Release 2>&1 | tail -30`
Expected: Build succeeds

**Step 5: Commit**

```bash
cd /Users/orperelman/Downloads/ProgFlow-JUCE
git add Source/UI/Synths/SynthEditorBase.h Source/UI/Synths/SynthEditorBase.cpp CMakeLists.txt
git commit -m "feat(ui): add SynthEditorBase class with shared layout logic"
```

---

## Phase 2: Refactor Synth Editors

### Task 4: Refactor AnalogSynthEditor

**Files:**
- Modify: `Source/UI/Synths/AnalogSynthEditor.h`
- Modify: `Source/UI/Synths/AnalogSynthEditor.cpp`

**Step 1: Update AnalogSynthEditor.h to inherit from SynthEditorBase**

Replace the file with:

```cpp
#pragma once

#include "SynthEditorBase.h"
#include "../../Audio/Synths/AnalogSynth.h"

class AnalogSynthEditor : public SynthEditorBase
{
public:
    explicit AnalogSynthEditor(AnalogSynth& synth);
    ~AnalogSynthEditor() override;

    void comboBoxChanged(juce::ComboBox* box) override;

protected:
    void layoutContent(juce::Rectangle<int> area) override;
    void drawDividers(juce::Graphics& g, juce::Rectangle<int> area) override;

private:
    AnalogSynth& synth;

    // OSC 1
    juce::Label osc1Label;
    juce::ComboBox osc1Wave;
    RotaryKnob osc1Semi;
    RotaryKnob osc1Fine;

    // OSC 2
    juce::Label osc2Label;
    juce::ComboBox osc2Wave;
    RotaryKnob osc2Semi;
    RotaryKnob osc2Detune;

    // Filter
    juce::Label filterLabel;
    juce::ComboBox filterType;
    RotaryKnob filterCutoff;
    RotaryKnob filterResonance;
    RotaryKnob filterEnvAmount;

    // Amp Envelope
    juce::Label ampEnvLabel;
    RotaryKnob ampAttack;
    RotaryKnob ampDecay;
    RotaryKnob ampSustain;
    RotaryKnob ampRelease;

    // Filter Envelope
    juce::Label filterEnvLabel;
    RotaryKnob filterAttack;
    RotaryKnob filterDecay;
    RotaryKnob filterSustain;
    RotaryKnob filterRelease;

    // Section x positions for dividers
    std::vector<int> sectionDividers;

    void setupKnob(RotaryKnob& knob, const juce::String& paramId,
                   const juce::String& label, const juce::String& suffix = "");
    void setupComboBox(juce::ComboBox& box, const juce::String& paramId);
    void populatePresets();
    void refreshFromSynth();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogSynthEditor)
};
```

**Step 2: Update AnalogSynthEditor.cpp**

Replace with new implementation using dividers instead of boxes (full file - see implementation):

```cpp
#include "AnalogSynthEditor.h"

AnalogSynthEditor::AnalogSynthEditor(AnalogSynth& s)
    : synth(s)
{
    // Set up preset selector
    presetSelector.addListener(this);
    populatePresets();

    // Master volume
    masterVolume.setLabel("Vol");
    if (auto* param = synth.getParameterInfo("volume"))
    {
        masterVolume.setRange(param->minValue, param->maxValue, param->step);
        masterVolume.setDefaultValue(param->defaultValue);
        masterVolume.setValue(param->value, juce::dontSendNotification);
    }
    masterVolume.onValueChange = [this](float value) {
        synth.setParameter("volume", value);
    };

    // OSC 1
    createSectionLabel(osc1Label, "OSC 1");
    addAndMakeVisible(osc1Label);
    setupComboBox(osc1Wave, "osc1_wave");
    setupKnob(osc1Semi, "osc1_semi", "Semi", " st");
    setupKnob(osc1Fine, "osc1_fine", "Fine", " ct");

    // OSC 2
    createSectionLabel(osc2Label, "OSC 2");
    addAndMakeVisible(osc2Label);
    setupComboBox(osc2Wave, "osc2_wave");
    setupKnob(osc2Semi, "osc2_semi", "Semi", " st");
    setupKnob(osc2Detune, "osc2_detune", "Detune", " ct");

    // Filter
    createSectionLabel(filterLabel, "FILTER");
    addAndMakeVisible(filterLabel);
    setupComboBox(filterType, "filter_type");
    setupKnob(filterCutoff, "filter_cutoff", "Cut", " Hz");
    setupKnob(filterResonance, "filter_resonance", "Res");
    setupKnob(filterEnvAmount, "filter_env_amount", "Env");

    // Amp Envelope
    createSectionLabel(ampEnvLabel, "AMP ENV");
    addAndMakeVisible(ampEnvLabel);
    setupKnob(ampAttack, "amp_attack", "A", " s");
    setupKnob(ampDecay, "amp_decay", "D", " s");
    setupKnob(ampSustain, "amp_sustain", "S");
    setupKnob(ampRelease, "amp_release", "R", " s");

    // Filter Envelope
    createSectionLabel(filterEnvLabel, "FLT ENV");
    addAndMakeVisible(filterEnvLabel);
    setupKnob(filterAttack, "filter_attack", "A", " s");
    setupKnob(filterDecay, "filter_decay", "D", " s");
    setupKnob(filterSustain, "filter_sustain", "S");
    setupKnob(filterRelease, "filter_release", "R", " s");

    refreshFromSynth();
}

AnalogSynthEditor::~AnalogSynthEditor()
{
    presetSelector.removeListener(this);
    osc1Wave.removeListener(this);
    osc2Wave.removeListener(this);
    filterType.removeListener(this);
}

void AnalogSynthEditor::setupKnob(RotaryKnob& knob, const juce::String& paramId,
                                   const juce::String& label, const juce::String& suffix)
{
    knob.setLabel(label);
    knob.setValueSuffix(suffix);

    if (auto* param = synth.getParameterInfo(paramId))
    {
        knob.setRange(param->minValue, param->maxValue, param->step);
        knob.setDefaultValue(param->defaultValue);
        knob.setValue(param->value, juce::dontSendNotification);
    }

    knob.onValueChange = [this, paramId](float value) {
        synth.setParameter(paramId, value);
    };

    addAndMakeVisible(knob);
}

void AnalogSynthEditor::setupComboBox(juce::ComboBox& box, const juce::String& paramId)
{
    if (auto* param = synth.getParameterInfo(paramId))
    {
        int id = 1;
        for (const auto& option : param->options)
            box.addItem(option, id++);
        box.setSelectedId(param->enumIndex + 1, juce::dontSendNotification);
    }

    box.addListener(this);
    addAndMakeVisible(box);
}

void AnalogSynthEditor::comboBoxChanged(juce::ComboBox* box)
{
    int index = box->getSelectedId() - 1;

    if (box == &presetSelector)
    {
        synth.loadPreset(index);
        refreshFromSynth();
    }
    else if (box == &osc1Wave)
        synth.setParameterEnum("osc1_wave", index);
    else if (box == &osc2Wave)
        synth.setParameterEnum("osc2_wave", index);
    else if (box == &filterType)
        synth.setParameterEnum("filter_type", index);
}

void AnalogSynthEditor::populatePresets()
{
    presetSelector.clear();
    auto presets = synth.getPresets();
    int id = 1;
    for (const auto& preset : presets)
        presetSelector.addItem(preset.name, id++);

    int currentPreset = synth.getCurrentPresetIndex();
    if (currentPreset >= 0)
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
    else if (!presets.empty())
    {
        synth.loadPreset(0);
        presetSelector.setSelectedId(1, juce::dontSendNotification);
    }
}

void AnalogSynthEditor::refreshFromSynth()
{
    auto refreshKnob = [this](RotaryKnob& knob, const juce::String& paramId) {
        if (auto* param = synth.getParameterInfo(paramId))
            knob.setValue(param->value, juce::dontSendNotification);
    };

    auto refreshCombo = [this](juce::ComboBox& box, const juce::String& paramId) {
        if (auto* param = synth.getParameterInfo(paramId))
            box.setSelectedId(param->enumIndex + 1, juce::dontSendNotification);
    };

    refreshKnob(masterVolume, "volume");
    refreshCombo(osc1Wave, "osc1_wave");
    refreshKnob(osc1Semi, "osc1_semi");
    refreshKnob(osc1Fine, "osc1_fine");
    refreshCombo(osc2Wave, "osc2_wave");
    refreshKnob(osc2Semi, "osc2_semi");
    refreshKnob(osc2Detune, "osc2_detune");
    refreshCombo(filterType, "filter_type");
    refreshKnob(filterCutoff, "filter_cutoff");
    refreshKnob(filterResonance, "filter_resonance");
    refreshKnob(filterEnvAmount, "filter_env_amount");
    refreshKnob(ampAttack, "amp_attack");
    refreshKnob(ampDecay, "amp_decay");
    refreshKnob(ampSustain, "amp_sustain");
    refreshKnob(ampRelease, "amp_release");
    refreshKnob(filterAttack, "filter_attack");
    refreshKnob(filterDecay, "filter_decay");
    refreshKnob(filterSustain, "filter_sustain");
    refreshKnob(filterRelease, "filter_release");

    int currentPreset = synth.getCurrentPresetIndex();
    if (currentPreset >= 0)
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
}

void AnalogSynthEditor::drawDividers(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Draw vertical dividers between sections
    for (int x : sectionDividers)
        drawVerticalDivider(g, x, area.getY(), area.getBottom());
}

void AnalogSynthEditor::layoutContent(juce::Rectangle<int> area)
{
    sectionDividers.clear();

    const int pad = SECTION_PADDING;
    const int labelH = 16;
    const int comboH = 24;
    const int knobH = RotaryKnob::TOTAL_HEIGHT;

    // 5 sections: OSC1 | OSC2 | FILTER | AMP ENV | FLT ENV
    // Widths: 15% | 15% | 20% | 25% | 25%
    int w1 = area.getWidth() * 15 / 100;
    int w2 = area.getWidth() * 15 / 100;
    int w3 = area.getWidth() * 20 / 100;
    int w4 = area.getWidth() * 25 / 100;
    int w5 = area.getWidth() - w1 - w2 - w3 - w4;

    int x = area.getX();
    int y = area.getY();
    int h = area.getHeight();

    // OSC 1
    {
        auto sec = juce::Rectangle<int>(x, y, w1, h).reduced(pad);
        osc1Label.setBounds(sec.removeFromTop(labelH));
        sec.removeFromTop(4);
        osc1Wave.setBounds(sec.removeFromTop(comboH));
        sec.removeFromTop(8);

        int kw = sec.getWidth() / 2;
        osc1Semi.setBounds(sec.removeFromLeft(kw).withHeight(knobH));
        osc1Fine.setBounds(sec.withHeight(knobH));
    }
    x += w1;
    sectionDividers.push_back(x);

    // OSC 2
    {
        auto sec = juce::Rectangle<int>(x, y, w2, h).reduced(pad);
        osc2Label.setBounds(sec.removeFromTop(labelH));
        sec.removeFromTop(4);
        osc2Wave.setBounds(sec.removeFromTop(comboH));
        sec.removeFromTop(8);

        int kw = sec.getWidth() / 2;
        osc2Semi.setBounds(sec.removeFromLeft(kw).withHeight(knobH));
        osc2Detune.setBounds(sec.withHeight(knobH));
    }
    x += w2;
    sectionDividers.push_back(x);

    // FILTER
    {
        auto sec = juce::Rectangle<int>(x, y, w3, h).reduced(pad);
        filterLabel.setBounds(sec.removeFromTop(labelH));
        sec.removeFromTop(4);
        filterType.setBounds(sec.removeFromTop(comboH));
        sec.removeFromTop(8);

        int kw = sec.getWidth() / 3;
        filterCutoff.setBounds(sec.removeFromLeft(kw).withHeight(knobH));
        filterResonance.setBounds(sec.removeFromLeft(kw).withHeight(knobH));
        filterEnvAmount.setBounds(sec.withHeight(knobH));
    }
    x += w3;
    sectionDividers.push_back(x);

    // AMP ENV
    {
        auto sec = juce::Rectangle<int>(x, y, w4, h).reduced(pad);
        ampEnvLabel.setBounds(sec.removeFromTop(labelH));
        sec.removeFromTop(4 + comboH + 8); // Align with other sections

        int kw = sec.getWidth() / 4;
        ampAttack.setBounds(sec.removeFromLeft(kw).withHeight(knobH));
        ampDecay.setBounds(sec.removeFromLeft(kw).withHeight(knobH));
        ampSustain.setBounds(sec.removeFromLeft(kw).withHeight(knobH));
        ampRelease.setBounds(sec.withHeight(knobH));
    }
    x += w4;
    sectionDividers.push_back(x);

    // FLT ENV
    {
        auto sec = juce::Rectangle<int>(x, y, w5, h).reduced(pad);
        filterEnvLabel.setBounds(sec.removeFromTop(labelH));
        sec.removeFromTop(4 + comboH + 8); // Align with other sections

        int kw = sec.getWidth() / 4;
        filterAttack.setBounds(sec.removeFromLeft(kw).withHeight(knobH));
        filterDecay.setBounds(sec.removeFromLeft(kw).withHeight(knobH));
        filterSustain.setBounds(sec.removeFromLeft(kw).withHeight(knobH));
        filterRelease.setBounds(sec.withHeight(knobH));
    }
}
```

**Step 3: Build and verify**

Run: `cd /Users/orperelman/Downloads/ProgFlow-JUCE/build && cmake --build . --config Release 2>&1 | tail -30`
Expected: Build succeeds

**Step 4: Test visually**

Run: `open /Users/orperelman/Downloads/ProgFlow-JUCE/build/ProgFlow_artefacts/Release/ProgFlow.app`
Expected: Analog synth editor shows tight layout with dividers instead of boxes

**Step 5: Commit**

```bash
cd /Users/orperelman/Downloads/ProgFlow-JUCE
git add Source/UI/Synths/AnalogSynthEditor.h Source/UI/Synths/AnalogSynthEditor.cpp
git commit -m "refactor(ui): AnalogSynthEditor uses SynthEditorBase with dividers"
```

---

### Task 5-10: Refactor remaining synth editors

Apply the same pattern to:
- **Task 5:** FMSynthEditor
- **Task 6:** PolyPadSynthEditor
- **Task 7:** OrganSynthEditor
- **Task 8:** StringSynthEditor
- **Task 9:** ProSynthEditor
- **Task 10:** DrumSynthEditor

Each follows the same steps:
1. Update .h to inherit from SynthEditorBase
2. Update .cpp to use layoutContent() and drawDividers()
3. Build and verify
4. Test visually
5. Commit

---

## Phase 3: Panels

### Task 11: Update TransportBar styling

**Files:**
- Modify: `Source/UI/Panels/TransportBar.cpp`

**Changes:**
- Height: 36px
- Background: `#252525`
- Tighter button grouping
- Custom toggle styling for Metro/Count/Loop

---

### Task 12: Update TrackHeaderPanel styling

**Files:**
- Modify: `Source/UI/Panels/TrackHeaderPanel.cpp`

**Changes:**
- Track height: 72px
- Selected track: 4px left border accent
- Mini sliders for volume/pan
- Tighter M/S/R buttons

---

### Task 13: Update TimelinePanel styling

**Files:**
- Modify: `Source/UI/Timeline/TimelinePanel.cpp`

**Changes:**
- Grid colors: `#2a2a2a` beats, `#3a3a3a` bars
- Playhead: 2px blue line
- Clip styling with track colors

---

### Task 14: Update MixerPanel styling

**Files:**
- Modify: `Source/UI/Panels/MixerPanel.cpp`

**Changes:**
- Channel width: 70px
- Vertical layout: name, meter, pan, fader, buttons
- Meter colors: green/yellow/red

---

### Task 15: Update PianoRollEditor styling

**Files:**
- Modify: `Source/UI/PianoRoll/PianoRollEditor.cpp`

**Changes:**
- Piano key colors
- Grid styling
- Note colors based on velocity
- Toolbar styling

---

### Task 16: Update WelcomeScreen styling

**Files:**
- Modify: `Source/UI/WelcomeScreen.cpp`

**Changes:**
- Primary button: solid blue
- Secondary button: outlined blue
- Consistent with app theme

---

## Final Steps

### Task 17: Full visual QA and polish

1. Launch app
2. Check each synth editor tab
3. Check transport bar
4. Check track list
5. Check timeline
6. Check mixer (if visible)
7. Check piano roll (open a clip)
8. Check welcome screen (close project)
9. Fix any spacing/alignment issues
10. Final commit

```bash
git add -A
git commit -m "feat(ui): complete UI redesign - Ableton-style clean interface"
```
