# ProgFlow "Saturn" UI Redesign

## Overview

Complete UI overhaul with FabFilter-inspired professional dark aesthetic and purple accent color. Also consolidating synths from 9 to 6.

## Synth Consolidation

### Remove (3)
- **PolyPad** - redundant with ProSynth
- **Organ** - redundant with ProSynth
- **String** - redundant with ProSynth

### Keep (6)
- **ProSynth** - main polysynth, versatile
- **FM** - unique FM synthesis sound
- **Analog** - classic subtractive
- **Drums** - essential for beats
- **Sampler** - essential for samples
- **SoundFont** - essential for SF2 playback

---

## Color Palette

```cpp
// Backgrounds (warm dark, not pure black)
bgPrimary      = 0xff1a1a1f   // Deep background
bgSecondary    = 0xff232328   // Panel backgrounds
bgSurface      = 0xff2a2a30   // Raised surfaces/cards
bgHover        = 0xff35353d   // Hover states

// Purple accent (Saturn-inspired)
accentPurple   = 0xff9d7cd8   // Primary accent
accentBright   = 0xffbb9af7   // Hover/active states
glowPurple     = 0x409d7cd8   // Bloom effects (25% opacity)

// Semantic colors
positive       = 0xff7dcfff   // Cyan - play, success
warning        = 0xffe0af68   // Gold - warnings, mute
negative       = 0xfff7768e   // Coral - record, errors

// Text
textPrimary    = 0xffe0e0e0   // Main text (not pure white)
textSecondary  = 0xff888890   // Labels
textMuted      = 0xff5a5a62   // Section headers

// Knob specific
knobBody       = 0xff2d2d35   // Knob background
knobBodyLight  = 0xff3d3d45   // Gradient highlight
knobArc        = 0xff9d7cd8   // Value arc (purple)
knobArcBg      = 0xff3a3a42   // Inactive arc
```

---

## Component Designs

### Knob (48px diameter)

**Visual:**
- Metallic gradient body (top-lit highlight)
- Thin arc indicator (2px) with soft glow when active
- Small white indicator dot
- No heavy outer glow - subtle and refined

**Layout:**
```
    [  knob  ]     48x48px
      Label        11px, textMuted
      1.50         12px, textPrimary
```

**States:**
- Default: metallic gradient, no glow
- Hover: thin purple border, slight brightening
- Dragging: purple arc glow, brighter
- MIDI mapped: small green dot in corner

### Panel Cards

Each control group in a card:
- Background: subtle gradient (top #2a2a30 → bottom #232328)
- Border: 1px rgba(255,255,255,0.05)
- Shadow: 0 4px 8px rgba(0,0,0,0.2)
- Corners: 6px radius
- Padding: 12px internal

**Header style:**
- 11px uppercase
- textMuted color
- 1px letter-spacing
- 8px bottom margin

### ComboBox

- Background: bgSurface
- Border: 1px border color
- Border on focus: accentPurple
- Rounded: 4px
- Arrow: small chevron, textSecondary

### Buttons (S/M/R)

- Size: 24x20px
- Rounded: 4px
- Off: bgSurface with border
- On colors:
  - Solo (S): positive (cyan)
  - Mute (M): warning (gold)
  - Record/Arm (R): negative (coral)

### Meters

- LED-style segments
- Gradient: green (#7dcfff) → yellow (#e0af68) → red (#f7768e)
- Background: bgPrimary
- Peak hold line
- Width: 4px per channel, 2px gap for stereo

---

## Layout: FM Synth Editor

```
┌─────────────────────────────────────────────────────────────────────┐
│ PRESET [▼ Electric Piano        ]                    VOLUME [knob] │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ╭─────────────╮ ╭─────────────╮ ╭─────────────╮ ╭───────────────╮ │
│  │ ALGORITHM   │ │   CARRIER   │ │ MODULATOR 1 │ │  MODULATOR 2  │ │
│  │             │ │             │ │             │ │               │ │
│  │ [▼ Serial ] │ │   [Ratio]   │ │[Ratio][Idx] │ │ [Ratio][Idx]  │ │
│  ╰─────────────╯ ╰─────────────╯ ╰─────────────╯ ╰───────────────╯ │
│                                                                     │
│  ╭─────────────╮ ╭─────────────────────────────────────────────────╮│
│  │  FEEDBACK   │ │                    ENVELOPES                    ││
│  │   [Amount]  │ │  AMP           MOD 1           MOD 2           ││
│  ╰─────────────╯ │  [A][D][S][R]  [A][D][S][R]   [A][D][S][R]     ││
│                  ╰─────────────────────────────────────────────────╯│
└─────────────────────────────────────────────────────────────────────┘
```

- Row 1: Operators (Algorithm, Carrier, Mod 1, Mod 2)
- Row 2: Feedback + Envelopes card (all 3 envelopes grouped)
- More vertical space, less cramped
- Each logical group in its own card

---

## Layout: Track List Panel

```
╭──────────────────────────╮
│ TRACKS              [+]  │
├──────────────────────────┤
│ ╭──────────────────────╮ │
│ │ ● Track 1            │ │  ← Color dot + name
│ │ FM Synth         ▼   │ │
│ │ [S] [M] [R]          │ │  ← Colored buttons
│ │ ───────●─────────    │ │  ← Horizontal volume
│ ╰──────────────────────╯ │
╰──────────────────────────╯
```

- Track color indicator (dot or strip)
- Each track in a card
- S/M/R with semantic colors
- Compact horizontal volume slider

---

## Layout: Transport Bar

```
╭──────────────────────────────────────────────────────────────────────╮
│ [⏮][▶][⏹][●]  │ BPM [120] │ [4/4▼] │ 01:02:345 │ ☐Metro ☐Loop │ CPU │
╰──────────────────────────────────────────────────────────────────────╯
```

- Play button: cyan fill when playing
- Record button: coral fill when recording
- Monospace font for time (fixed width digits)
- Subtle card background
- Clean spacing

---

## Layout: Mixer Panel

```
╭────────╮ ╭────────╮ ╭────────╮       ╭──────────╮
│Track 1 │ │Track 2 │ │Track 3 │  ...  │  MASTER  │
│  ▮▮    │ │  ▮▮    │ │  ▮▮    │       │   ▮▮▮▮   │
│  ──●── │ │  ──●── │ │  ──●── │       │   ──●──  │
│  -6dB  │ │   0dB  │ │ -12dB  │       │    0dB   │
│ [S][M] │ │ [S][M] │ │ [S][M] │       │          │
╰────────╯ ╰────────╯ ╰────────╯       ╰──────────╯
```

- Each channel in a card
- LED meters with peak hold
- Vertical fader with purple accent
- Master section: wider, visually separated
- dB readout under fader

---

## Implementation Order

### Phase 1: Foundation
1. Update LookAndFeel with new color palette
2. Redesign RotaryKnob component (48px, metallic, value display)
3. Create CardPanel component (gradient bg, shadow, rounded)
4. Style ComboBox and buttons

### Phase 2: Remove Synths
5. Remove PolyPad synth (files + references)
6. Remove Organ synth (files + references)
7. Remove String synth (files + references)
8. Update synth type enum and factory

### Phase 3: FM Editor
9. Refactor FMSynthEditor with cards layout
10. Test and polish FM editor

### Phase 4: Other Synth Editors
11. AnalogSynthEditor redesign
12. ProSynthEditor redesign
13. DrumSynthEditor redesign
14. SamplerEditor redesign
15. SoundFontPlayerEditor redesign

### Phase 5: Main Panels
16. Track List panel redesign
17. Transport bar redesign
18. Mixer panel redesign
19. Timeline/Arrangement polish

### Phase 6: Final Polish
20. Consistent spacing/alignment pass
21. Hover/focus states everywhere
22. Final testing and tweaks
