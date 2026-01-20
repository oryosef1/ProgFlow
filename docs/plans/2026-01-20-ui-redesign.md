# ProgFlow UI Redesign

## Overview

Complete UI redesign following Ableton-style aesthetics: dense, efficient, clean, easy to use and pleasant to look at. Eliminates the current issue of tiny controls floating in massive empty boxes.

## Design Principles

1. **Tight parameter grouping** - Controls sit close together, no wasted space
2. **Horizontal flow** - Parameters read left-to-right in logical groups
3. **Subtle visual separation** - Thin lines (1-2px), not giant boxes
4. **Consistent sizing** - All knobs same size, all labels same style
5. **Clear hierarchy** - Section headers, then controls, then labels

---

## Core Visual System

### Colors (Dark Theme)

```cpp
// Backgrounds
bgPrimary      = #1a1a1a   // Main panel background
bgSecondary    = #252525   // Transport bar, raised surfaces
bgTertiary     = #2a2a2a   // Hover states, subtle highlights
sectionBg      = #1e1e1e   // Inset areas (removed - using dividers instead)
dividerLine    = #2a2a2a   // Section dividers (1-2px lines)

// Accents
accentBlue     = #3b82f6   // Primary accent, selected items, knob arcs
accentGreen    = #22c55e   // Play button, positive meters
accentOrange   = #f59e0b   // Warnings, caution
accentRed      = #ef4444   // Record, errors, clip meters

// Text
textPrimary    = #ffffff   // Values, important text
textSecondary  = #cccccc   // Track names, labels
textMuted      = #888888   // Section headers
textDisabled   = #555555   // Inactive controls

// Components
knobBody       = #3a3a3a   // Knob background
knobArcInactive= #4a4a4a   // Unlit portion of arc
knobArcActive  = #3b82f6   // Lit portion of arc
```

### Spacing Constants

```cpp
namespace ProgFlowSpacing {
    XS = 4;    // Tight spacing (label to knob)
    SM = 8;    // Between related controls
    MD = 12;   // Section internal padding
    LG = 16;   // Between sections
    XL = 24;   // Major separations

    DIVIDER_WIDTH = 1;         // Thin divider lines
    KNOB_SIZE = 48;            // Knob diameter
    KNOB_WITH_LABEL = 68;      // Knob + gap + label height
    SECTION_HEADER = 16;       // Section header text height
}
```

### Component Sizes

- **Knob diameter**: 48px
- **Knob total with label**: 68px (48 + 4 gap + 16 label)
- **Section header text**: 10px, uppercase, muted color
- **Value/label text**: 11px
- **ComboBox height**: 24px
- **Button height**: 28px (transport), 36px (welcome screen)

---

## Transport Bar

Height: 36px, background: `#252525`

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ▶ ■ ●  │  BPM [120] Tap  │  4/4  │  1:1:000  │  □Metro □Count □Loop  │ CPU │
└─────────────────────────────────────────────────────────────────────────────┘
```

- Play/Stop/Record: 24x24px icons, tight grouping
- BPM: Editable field, subtle background
- Position: Monospace font `1:1:000`
- Toggles: Custom styled (not default JUCE checkboxes)
- Dividers: Thin vertical lines `#3a3a3a`

---

## Track List Panel (Left Sidebar)

Width: 160px

### Track Item (72px height)

```
┌─────────────────────────────┐
│ Track 1                   A │  <- Name + Arm indicator
│ [Analog            ▼]       │  <- Synth selector
│ M  S  R  │ ▁▁█▁ │ ◀──●──▶  │  <- Mute/Solo/Rec, Volume, Pan
└─────────────────────────────┘
```

- Selected track: 4px left border `#3b82f6`
- Hover: Background `#2a2a2a`
- M/S/R buttons: 20x20px toggles
- Volume/Pan: Mini horizontal sliders (40px each)

### Master Section

- Always visible at bottom
- Stereo meter bars (vertical)
- Master volume knob

---

## Timeline / Arrangement View

### Bar Ruler
- Bar numbers at each bar
- Minor ticks at beats
- Height: 24px

### Track Lanes
- Aligned with track list
- Grid lines: `#2a2a2a` at beats, `#3a3a3a` at bars

### Clips
- Rounded rectangles (4px radius)
- Background: Track color at 60% opacity
- Border: Track color solid, 1px
- Name: Small text top-left
- MIDI preview: Tiny visualization inside

### Playhead
- Vertical line `#3b82f6`
- Triangle indicator at top
- Width: 2px

---

## Synth Editor Panel

Height: ~280px, background: `#1a1a1a`

### Header Row (40px)

```
│ PRESET [Dropdown────────] ◀ ▶    ───────────    VOL [◐] │
```

- Preset dropdown: Left aligned
- Prev/Next buttons: Optional
- Master volume: Right aligned, always visible

### Content Layout

**Key change**: No rounded rectangle boxes. Use thin vertical/horizontal divider lines (1px, `#2a2a2a`) to separate sections.

Each section has:
- Section header (10px, uppercase, muted)
- Controls below
- Labels under controls

### FM Synth Layout

```
│ ALGORITHM      │ CARRIER │ MOD 1       │ MOD 2       │ FEEDBACK │
│ [Serial 2→1→C] │   ◐     │  ◐    ◐    │  ◐    ◐    │    ◐     │
│                │  Ratio  │ Ratio Index│ Ratio Index │   Amt    │
├────────────────┴─────────┴────────────┴─────────────┴──────────┤
│ AMP ENVELOPE        │ MOD 1 ENVELOPE      │ MOD 2 ENVELOPE     │
│  ◐    ◐    ◐    ◐  │  ◐    ◐    ◐    ◐  │  ◐    ◐    ◐    ◐  │
│  A    D    S    R  │  A    D    S    R  │  A    D    S    R  │
```

### Analog Synth Layout

```
│ OSC 1      │ OSC 2       │ FILTER          │ AMP ENV       │ FILTER ENV    │
│ [Wave ▼]   │ [Wave ▼]    │ [Type ▼]        │ ◐  ◐  ◐  ◐   │ ◐  ◐  ◐  ◐   │
│  ◐   ◐     │  ◐    ◐     │  ◐    ◐    ◐   │ A  D  S  R   │ A  D  S  R   │
│ Semi Fine  │ Semi Detune │ Cut  Res  Env  │              │              │
```

### PolyPad Synth Layout

```
│ OSCILLATORS        │ FILTER          │ CHORUS      │
│ [Wave1▼] [Wave2▼]  │ [Type ▼]        │  ◐   ◐   ◐  │
│   ◐        ◐       │  ◐    ◐    ◐   │ Rate Dep Mix│
│  Detune   Mix      │ Cut  Res  Env  │             │
├────────────────────┴─────────────────┴─────────────┤
│ AMP ENVELOPE            │ FILTER ENVELOPE          │
│  ◐    ◐    ◐    ◐      │  ◐    ◐    ◐    ◐      │
│  A    D    S    R      │  A    D    S    R      │
```

### Organ Synth Layout

```
│ DRAWBARS                                              │
│  ◐   ◐   ◐   ◐   ◐   ◐   ◐   ◐   ◐                  │
│ 16' 5⅓' 8'  4' 2⅔' 2' 1⅗'1⅓' 1'                     │
├───────────────────────────────────────────────────────┤
│ PERCUSSION      │ ROTARY       │ DRIVE  │ KEY CLICK │
│ [On/Off▼]       │ [Speed ▼]    │   ◐    │    ◐      │
│ [Decay▼][Harm▼] │     ◐        │  Amt   │   Amt     │
│                 │   Depth      │        │           │
```

### Other Synths

String, Pro, Drum synths follow the same pattern:
- Logical horizontal groupings
- Two rows if needed
- Thin dividers between sections

---

## Mixer Panel

Channel width: 70px, Master: 90px

```
┌─────────┐
│ Track 1 │  <- Name
│ [Analog]│  <- Synth type
├─────────┤
│  ▒▒▒▒   │  <- Stereo meter (80px tall)
│  ████   │
├─────────┤
│   ◐     │  <- Pan knob
├─────────┤
│   █     │  <- Volume fader (100px tall)
│   █     │
├─────────┤
│ M  S  R │  <- Mute/Solo/Record
└─────────┘
```

### Meter Colors
- Green: -∞ to -12dB
- Yellow: -12dB to -6dB
- Red: -6dB to 0dB

---

## Piano Roll Editor

Opens in bottom panel when double-clicking a clip.

### Toolbar (32px)

```
│ ✕ Close │ [Snap: 1/16 ▼] │ [Tool: Draw ▼] │ Vel │ Quantize │
```

### Main Grid

- Piano keys (left): 40px wide, black keys `#1a1a1a`, white `#3a3a3a`
- Grid: Beat lines `#2a2a2a`, bar lines `#3a3a3a`
- Notes: Rounded rectangles, blue `#3b82f6`, velocity affects brightness

### Velocity Lane (bottom, 60px)

- Vertical bars for each note
- Height = velocity

### Note Styling

- Base color: `#3b82f6`
- Velocity: 40% brightness (soft) to 100% (loud)
- Selected: White border 2px

---

## Welcome Screen

Centered layout, clean buttons.

```
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│                         ♪ ProgFlow                          │
│                    Music production made simple             │
│                                                             │
│            [  Create New Project  ]  [  Open Project  ]     │
│                                                             │
├─────────────────────────────────────────────────────────────┤
│  RECENT PROJECTS                                            │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ ● My Song                              ~/Music/       │  │
│  │ ● Demo Track                           ~/Projects/    │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
│              ⌘S Save  │  Space Play/Pause  │  ⌘Z Undo      │
└─────────────────────────────────────────────────────────────┘
```

### Button Styles
- Primary (Create New): Solid `#3b82f6`, white text, 180x44px
- Secondary (Open): Transparent, `#3b82f6` border, blue text

---

## Implementation Plan

### Order of Work

1. **Update RotaryKnob** - Fixed 48px size, cleaner arc rendering
2. **Update LookAndFeel** - New colors, remove box drawing, add divider support
3. **Create SynthEditorBase** - Shared layout logic, header row, divider drawing
4. **Refactor AnalogSynthEditor** - Test new pattern
5. **Apply to remaining synths** - FM, PolyPad, Organ, String, Pro, Drum
6. **Update Track List Panel** - Tighter layout, mini controls
7. **Update Transport Bar** - Compact styling, custom toggles
8. **Update Timeline** - Grid, clips, playhead styling
9. **Update Mixer Panel** - Channel strips, meters
10. **Update Piano Roll** - Grid, notes, toolbar
11. **Polish Welcome Screen** - Button styling

### Files to Modify

**Core:**
- `LookAndFeel.h/cpp` - Colors, spacing, component styling
- `RotaryKnob.h/cpp` - Fixed sizing, arc rendering

**New:**
- `SynthEditorBase.h/cpp` - Shared synth editor layout

**Synth Editors:**
- `AnalogSynthEditor.cpp`
- `FMSynthEditor.cpp`
- `PolyPadSynthEditor.cpp`
- `OrganSynthEditor.cpp`
- `StringSynthEditor.cpp`
- `ProSynthEditor.cpp`
- `DrumSynthEditor.cpp`

**Panels:**
- `TrackHeaderPanel.cpp`
- `TransportBar.cpp`
- `TimelinePanel.cpp`
- `MixerPanel.cpp`
- `PianoRollEditor.cpp`
- `WelcomeScreen.cpp`

---

## Success Criteria

- [ ] No more tiny controls in giant boxes
- [ ] Consistent 48px knobs throughout
- [ ] Clear visual hierarchy with thin dividers
- [ ] Readable labels and values
- [ ] Tight, efficient use of space
- [ ] Professional Ableton-like aesthetic
- [ ] All synth editors follow same pattern
- [ ] Transport, tracks, timeline, mixer all polished
