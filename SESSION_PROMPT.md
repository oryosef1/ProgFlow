# Session Prompt - Copy this to start a new session

---

## Context

ProgFlow is a DAW built with JUCE. We just finished planning a major UI redesign called "Saturn" - a FabFilter-inspired professional dark theme with purple accents.

**Key decisions made:**
1. Remove 3 redundant synths (PolyPad, Organ, String) - keep 6
2. FabFilter-style professional dark aesthetic
3. Purple accent color (like FabFilter Saturn)
4. New components: CardPanel, redesigned knobs with value display

## What to do

Start implementing **Phase 1: Foundation** from the Saturn UI redesign plan.

**Phase 1 tasks:**
1. Update LookAndFeel with Saturn color palette (purple accents, warm darks)
2. Redesign RotaryKnob (48px, metallic gradient, value display below)
3. Create CardPanel component (gradient bg, soft shadow, 6px corners)
4. Style ComboBox (purple focus border, custom look)
5. Style S/M/R buttons (Solo=cyan, Mute=gold, Record=coral)

## Key files to read first

- `docs/plans/2026-01-20-saturn-ui-redesign.md` - Full design spec with colors, component designs, layouts
- `todo.md` - Task tracking
- `Source/UI/LookAndFeel.cpp` - Current theme (needs Saturn colors)
- `Source/UI/Common/RotaryKnob.cpp` - Current knob (needs redesign)
- `Source/UI/Common/GlassPanel.cpp` - Reference for CardPanel

## Color palette (from design doc)

```cpp
// Backgrounds
bgPrimary      = 0xff1a1a1f   // Deep background
bgSecondary    = 0xff232328   // Panel backgrounds
bgSurface      = 0xff2a2a30   // Cards/raised surfaces

// Purple accent
accentPurple   = 0xff9d7cd8   // Primary
accentBright   = 0xffbb9af7   // Hover/active
glowPurple     = 0x409d7cd8   // Bloom (25% opacity)

// Semantic
positive       = 0xff7dcfff   // Cyan (play, solo)
warning        = 0xffe0af68   // Gold (mute)
negative       = 0xfff7768e   // Coral (record)

// Text
textPrimary    = 0xffe0e0e0
textSecondary  = 0xff888890
textMuted      = 0xff5a5a62
```

## Commands

```bash
# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release

# Run
open build/ProgFlow_artefacts/Release/ProgFlow.app

# Tests
./build/ProgFlowTests_artefacts/Release/ProgFlowTests
```

---

**Start with:** "Implement Phase 1 of the Saturn UI redesign. Read the design doc first, then update LookAndFeel colors, redesign the RotaryKnob, and create the CardPanel component."
