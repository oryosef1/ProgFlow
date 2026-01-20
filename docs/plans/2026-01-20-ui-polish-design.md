# UI Polish Design - Ableton Style

**Date:** 2026-01-20
**Goal:** Make ProgFlow look professional with clean, minimal Ableton-style UI

---

## Design Principles

- Clean, minimal, flat colors
- Grid-based layouts with consistent spacing
- Lots of whitespace/breathing room
- Rely on background contrast, not borders
- Consistent component sizes across all editors

---

## Color System

### Existing (keep)
```cpp
bgPrimary:     #1a1a1a  // Main background
bgSecondary:   #242424  // Panels
bgTertiary:    #2d2d2d  // Controls
accentBlue:    #3b82f6  // Primary accent
accentGreen:   #10b981  // Success/active
accentOrange:  #f59e0b  // Warning/learning
accentRed:     #ef4444  // Error/record
textPrimary:   #ffffff  // Main text
textSecondary: #a0a0a0  // Labels
```

### New colors to add
```cpp
sectionBg:     #141414  // Darker inset sections
sectionBgAlt:  #1e1e1e  // Slightly lighter variant
surfaceBg:     #222222  // Raised surfaces
textMuted:     #666666  // Section headers, less important
meterBg:       #0a0a0a  // Very dark meter background
```

---

## Spacing System

```cpp
static constexpr int SPACING_XS = 4;
static constexpr int SPACING_SM = 8;
static constexpr int SPACING_MD = 16;
static constexpr int SPACING_LG = 24;
static constexpr int SPACING_XL = 32;
```

All spacing should use these values. No arbitrary pixel values.

---

## Typography

| Element | Size | Weight | Color | Notes |
|---------|------|--------|-------|-------|
| Section header | 10px | Regular | textMuted | Uppercase, 1px letter-spacing |
| Control label | 11px | Regular | textSecondary | Below knobs |
| Value display | 11px | Regular | textSecondary | Monospace, brightens when active |
| Preset name | 13px | Regular | textPrimary | |
| Button text | 12px | Regular | textPrimary | |

---

## Components

### RotaryKnob

**Sizes:**
```cpp
static constexpr int KNOB_MIN_SIZE = 56;   // Minimum
static constexpr int KNOB_PREFERRED = 64;  // Ideal with label
```

**Visual:**
- Background: sectionBg (#141414), no border
- Value arc: 3px stroke, accent blue, starts at 225°
- Pointer: 2px white line from center to arc edge
- No dot at pointer end

**States:**
- Default: Arc shows value, label visible, value hidden
- Hover: Subtle 1px accent glow, value appears
- Dragging: Arc brighter, value text white
- MIDI mapped: Green border
- MIDI learning: Orange border

**Layout:**
```
┌─────────────────┐
│                 │
│    [  knob  ]   │  56px min height
│                 │
│     Label       │  11px, textSecondary
│     0.50        │  11px mono, only on hover/drag
└─────────────────┘
```

### SectionPanel

New component for consistent section styling.

**Structure:**
```
┌──────────────────────────────────────────┐
│  SECTION TITLE                           │  <- 10px uppercase, textMuted
├──────────────────────────────────────────┤
│                                          │
│   [content area - 16px padding]          │
│                                          │
└──────────────────────────────────────────┘
```

**Styling:**
- Background: sectionBg (#141414)
- Corner radius: 6px
- Padding: 16px all sides
- Header: top-left inside padding
- No border

### Meters

**Styling:**
```
Width: 12px per channel
Gap: 2px between L/R
Corner radius: 2px
Background: meterBg (#0a0a0a)

Colors by level:
- 0-70%:  meterGreen (#22c55e)
- 70-90%: meterYellow (#eab308)
- 90%+:   meterRed (#ef4444)
```

**Peak hold:** Thin 2px line at max level, fades over 1 second

---

## Synth Editor Layout

**Overall structure:**
```
┌─────────────────────────────────────────────────────────────────┐
│ PRESET ▼ [Preset Name          ]                    [Vol knob]  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐       │
│  │  OSC 1    │ │  OSC 2    │ │  OSC 3    │ │   SUB     │       │
│  │  [knobs]  │ │  [knobs]  │ │  [knobs]  │ │  [knobs]  │       │
│  └───────────┘ └───────────┘ └───────────┘ └───────────┘       │
│                                                                 │
│  ┌─────────────────┐ ┌───────────┐ ┌───────────────────┐       │
│  │     FILTER      │ │  AMP ENV  │ │    FILTER ENV     │       │
│  │     [knobs]     │ │  A D S R  │ │      A D S R      │       │
│  └─────────────────┘ └───────────┘ └───────────────────┘       │
│                                                                 │
│  ┌───────────────────────┐ ┌───────────────────────────┐       │
│  │    LFO 1 + LFO 2      │ │     GLIDE / UNISON        │       │
│  └───────────────────────┘ └───────────────────────────┘       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

**Layout rules:**
- 16px padding around entire editor
- 8px gap between sections
- Sections in same row: equal height
- Knobs within section: 8px horizontal gap
- Sections flex to fill width proportionally

**Preset bar:**
- Height: 40px
- Preset dropdown left, master volume right
- Subtle bottom border (#2a2a2a)

---

## Implementation Order

### Phase 1: Foundation
1. Add new colors to ThemeManager
2. Add spacing constants
3. Update LookAndFeel base styling (remove borders, etc.)

### Phase 2: Components
4. Update RotaryKnob (size, visuals, hover state)
5. Create SectionPanel component
6. Update meter styling

### Phase 3: Editors
7. Update AnalogSynthEditor layout (template)
8. Apply same layout to other synth editors
9. Update TransportBar meters

### Phase 4: Polish
10. Fine-tune spacing/alignment
11. Test all editors
12. Fix any edge cases

---

## Files to Modify

| File | Changes |
|------|---------|
| `Source/UI/LookAndFeel.h` | Add new colors, spacing constants |
| `Source/UI/LookAndFeel.cpp` | Update color values, remove borders |
| `Source/UI/Common/RotaryKnob.cpp` | New sizing, visuals, states |
| `Source/UI/Common/RotaryKnob.h` | Size constants |
| `Source/UI/Common/SectionPanel.cpp` | NEW - section container |
| `Source/UI/Common/SectionPanel.h` | NEW - section container |
| `Source/UI/Synths/AnalogSynthEditor.cpp` | New layout using sections |
| `Source/UI/Synths/*.cpp` | Apply same pattern to all editors |
| `Source/UI/TransportBar.cpp` | Meter updates |
