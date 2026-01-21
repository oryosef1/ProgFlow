# Session Prompt - Copy this to start a new session

---

## Context

ProgFlow is a DAW built with JUCE. We're implementing the "Saturn" UI redesign - a FabFilter-inspired professional dark theme with purple accents.

**Completed:**
- ✓ Phase 1: Foundation (Saturn colors, 48px knobs with value display, CardPanel, ComboBox, S/M/R buttons)
- ✓ Phase 2: Removed 3 redundant synths (PolyPad, Organ, String) - now 6 synths
- ✓ Phase 3: FM Editor redesigned with CardPanel layout

**Current synths (6):** Analog, FM, Pro, Sampler, SoundFont, Drums

## What to do

Continue with **Phase 4: Other Synth Editors** - apply the CardPanel layout to remaining synth editors.

**Phase 4 tasks:**
1. AnalogSynthEditor redesign
2. ProSynthEditor redesign
3. DrumSynthEditor redesign
4. SamplerEditor redesign
5. SoundFontPlayerEditor redesign

Use FMSynthEditor as the template - it shows how to use CardPanels to group related controls.

## Key files

- `todo.md` - Task tracking
- `docs/plans/2026-01-20-saturn-ui-redesign.md` - Full design spec
- `Source/UI/Synths/FMSynthEditor.cpp` - **Template** for CardPanel layout
- `Source/UI/Common/CardPanel.cpp` - CardPanel component
- `Source/UI/Common/RotaryKnob.cpp` - 48px knob with value display

## Saturn Design Reference

**CardPanel usage:**
```cpp
CardPanel myCard{"SECTION NAME"};
addAndMakeVisible(myCard);
myCard.addAndMakeVisible(myKnob);
// In layoutContent:
myCard.setBounds(bounds);
auto content = myCard.getContentArea();
myKnob.setBounds(content.withSizeKeepingCentre(KNOB_SIZE, RotaryKnob::TOTAL_HEIGHT));
```

**Knob sizes:**
- KNOB_SIZE = 48px diameter
- RotaryKnob::TOTAL_HEIGHT = 80px (knob + label + value)

**Colors (already implemented):**
- Purple accent: `ProgFlowColours::accentBlue()` (0xff9d7cd8)
- Cyan (solo/play): `ProgFlowColours::accentGreen()` (0xff7dcfff)
- Gold (mute): `ProgFlowColours::accentOrange()` (0xffe0af68)
- Coral (record): `ProgFlowColours::accentRed()` (0xfff7768e)

## Commands

```bash
# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release

# Run
build/ProgFlow_artefacts/Release/ProgFlow.app/Contents/MacOS/ProgFlow &

# Tests
./build/ProgFlowTests_artefacts/Release/ProgFlowTests
```

---

**Start with:** "Continue Phase 4 of the Saturn UI redesign. Read FMSynthEditor.cpp as a template, then redesign AnalogSynthEditor with CardPanels."
