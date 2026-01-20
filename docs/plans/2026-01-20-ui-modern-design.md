# ProgFlow Modern UI Design

## Overview

Complete UI redesign with modern, premium aesthetics inspired by the best DAW and plugin designs (Serum 2, Bitwig, Logic Pro). Features glassmorphism, soft depth, glowing accents, and resizable panels.

## Design Principles

1. **Modern & Beautiful** - Not just functional, but premium looking
2. **Depth, not flat** - Layered surfaces with soft shadows
3. **Glassmorphism** - Frosted glass panels with subtle blur
4. **Glowing accents** - Soft bloom on active elements
5. **Resizable panels** - All major panels can be resized by dragging
6. **Smooth animations** - 150ms transitions, fluid interactions

---

## Color Palette

### Dark Theme
```cpp
// Backgrounds (with depth)
bgPrimary      = #0d1117   // Deep background (subtle blue-black)
bgSecondary    = #161b22   // Panel backgrounds
bgTertiary     = #1f2428   // Hover states
bgGlass        = #ffffff10 // Frosted glass overlay (10% white)
bgGlassHover   = #ffffff15 // Glass hover state

// Accents (vibrant)
accentBlue     = #4C9EFF   // Primary - selections, knob arcs
accentGreen    = #3DDC84   // Play, positive indicators
accentOrange   = #FFAB40   // Warnings
accentRed      = #FF5252   // Record, errors

// Glow variants (for bloom effects)
glowBlue       = #4C9EFF40 // 25% opacity for glow
glowGreen      = #3DDC8440
glowOrange     = #FFAB4040
glowRed        = #FF525240

// Text
textPrimary    = #FFFFFF   // Values, important
textSecondary  = #B0B0B0   // Labels
textMuted      = #666666   // Section headers
textDisabled   = #444444   // Inactive

// Borders
borderSubtle   = #ffffff10 // Soft panel borders
borderGlow     = #4C9EFF30 // Glowing borders on focus
```

---

## Component Specifications

### ModernKnob (52px)
- **Size**: 52px diameter
- **Body**: Radial gradient (#2a2a2a center → #1a1a1a edge)
- **Ring**: 2px subtle border (#ffffff10)
- **Arc track**: 3px, #333333 (inactive portion)
- **Arc value**: 3px, accent color with soft glow
- **Indicator**: Small dot or line showing position
- **Label**: Below knob, textSecondary, 11px
- **Value**: Shows on hover/drag, textPrimary, 12px

### GlassPanel
- **Background**: bgGlass (#ffffff10) with blur simulation
- **Border**: 1px borderSubtle (#ffffff10)
- **Corner radius**: 8px
- **Shadow**: Soft drop shadow (0 4px 12px #00000040)
- **On focus**: Border becomes borderGlow

### ResizablePanel
- **Drag handle**: 4px zone at panel edge
- **Cursor**: resize cursor on hover
- **Handle visual**: Subtle line, brightens on hover
- **Min/max**: Configurable per panel
- **Persistence**: Saves size to preferences
- **Double-click**: Toggle min/default size

### EnvelopeVisualizer
- **Size**: ~100x50px
- **Background**: Subtle darker area
- **Curve**: Accent color line, 2px
- **Fill**: Accent color at 20% opacity below curve
- **Updates**: Real-time as ADSR knobs change

---

## Panel Layouts

### Synth Editor Panel

**Header Row (44px)**
```
┌─────────────────────────────────────────────────────────────────────────┐
│ [Synth Type ▼]  [Preset Name ▼] ◀ ▶         ─────────      [◐] Volume  │
└─────────────────────────────────────────────────────────────────────────┘
```
- Frosted glass background
- Synth selector: Left
- Preset selector: Next to synth
- Prev/Next arrows: Optional
- Volume knob: Right aligned

**Content Area**
- Section cards with glass effect
- Thin dividers between major sections
- Knobs in logical groups
- Labels directly under controls

**FM Synth Specific**
```
┌─ ALGORITHM ─┐┌─ CARRIER ─┐┌── MOD 1 ──┐┌── MOD 2 ──┐┌─ FEEDBACK ─┐
│ [Serial▼]   ││    ◐      ││  ◐    ◐   ││  ◐    ◐   ││     ◐      │
│ [diagram]   ││  Ratio    ││Ratio Index││Ratio Index││   Amount   │
└─────────────┘└───────────┘└───────────┘└───────────┘└────────────┘
┌──── AMP ENVELOPE ────┐┌─── MOD 1 ENVELOPE ──┐┌─── MOD 2 ENVELOPE ─┐
│  ◐    ◐    ◐    ◐    ││  ◐    ◐    ◐    ◐   ││  ◐    ◐    ◐    ◐  │
│  A    D    S    R    ││  A    D    S    R   ││  A    D    S    R  │
│ [envelope curve]     ││ [envelope curve]    ││ [envelope curve]   │
└──────────────────────┘└─────────────────────┘└────────────────────┘
```

---

## Resize Behavior

| Panel | Edge | Min | Max | Default |
|-------|------|-----|-----|---------|
| Synth Editor | Top | 200px | 60% window | 280px |
| Track List | Right | 120px | 300px | 180px |
| Mixer | Splitter with Timeline | 150px | 70% | 50% |
| Piano Roll | Top | 200px | 80% | 300px |

---

## Animation Specs

- **Hover transitions**: 150ms ease-out
- **Knob arc changes**: 100ms
- **Panel resize**: Real-time (no animation)
- **Focus glow**: 200ms fade in/out
- **Button press**: 50ms scale(0.98)

---

## Implementation Order

1. **LookAndFeel** - Update colors, add gradient/glow utilities
2. **ModernKnob** - New knob component with all features
3. **GlassPanel** - Reusable glass effect panel
4. **EnvelopeVisualizer** - ADSR curve display
5. **ResizablePanel** - Base class for resizable panels
6. **FMSynthEditor** - Apply everything, test the design
7. **Remaining synth editors** - Apply same pattern
8. **Other panels** - Track list, transport, timeline, mixer, piano roll

---

## Files to Create

| File | Description |
|------|-------------|
| `Source/UI/Common/ModernKnob.h/cpp` | New knob component |
| `Source/UI/Common/GlassPanel.h/cpp` | Glass effect panel |
| `Source/UI/Common/EnvelopeVisualizer.h/cpp` | ADSR curve display |
| `Source/UI/Common/ResizablePanel.h/cpp` | Resize functionality |

## Files to Modify

| File | Changes |
|------|---------|
| `Source/UI/LookAndFeel.h/cpp` | New colors, gradients, glow effects |
| `Source/UI/Synths/SynthEditorBase.h/cpp` | New layout system |
| `Source/UI/Synths/FMSynthEditor.cpp` | Apply new design |
| `Source/MainWindow.cpp` | Integrate resizable panels |
| `CMakeLists.txt` | Add new source files |

---

## Success Criteria

- [ ] All panels resizable with smooth drag
- [ ] Knobs look premium with glowing arcs
- [ ] Glass effect on panel headers
- [ ] Envelope visualizers update in real-time
- [ ] Smooth 150ms animations throughout
- [ ] Consistent look across all synth editors
- [ ] App feels modern and beautiful
