# ProgFlow Improvement Plan

## Overview
This plan covers making ProgFlow look professional and fixing all known issues.

---

## Phase 1: Critical Functionality Fixes

### 1.1 Project Management
- [ ] **Add project rename UI** - Click on project name in transport bar to edit
- [ ] **Test save/load** - Verify Cmd+S, Cmd+O work correctly
- [ ] **Fix "Untitled *" display** - Show dirty state properly

### 1.2 Audio Issues
- [x] ~~Drum kit switching only updates 4 pads~~ (FIXED)
- [x] ~~Quiet drum sounds (Rim, Shaker, Cymbal, Clap)~~ (FIXED)
- [ ] **SoundFont synth** - No .sf2 files bundled, needs default soundfont
- [ ] **Sampler** - No samples bundled, needs sample loading UI improvement
- [ ] **Verify all synths produce sound** - Test each one with virtual keyboard

### 1.3 Preset System
- [ ] Investigate preset loading issues
- [ ] Verify presets save/load correctly
- [ ] Add preset management (save custom, delete, organize)

---

## Phase 2: UI Polish - Transport Bar

### Current Issues:
- Basic appearance
- Project name not editable
- Meters are small

### Improvements:
- [ ] **Larger, cleaner transport buttons** - Play/Stop/Record with better icons
- [ ] **Editable project name** - Click to rename
- [ ] **Better time display** - Larger font, clearer bars:beats:ticks
- [ ] **Improved meters** - Taller, with peak hold indicators
- [ ] **Add tempo tap visual feedback** - Flash on tap

---

## Phase 3: UI Polish - Track List

### Current Issues:
- Track headers look cramped
- S/R/A buttons tiny
- Volume/Pan sliders hard to use

### Improvements:
- [ ] **Larger track headers** - More padding, cleaner layout
- [ ] **Better S/M/R buttons** - Solo (yellow), Mute (red), Record (red circle)
- [ ] **Improved volume/pan controls** - Larger knobs or sliders
- [ ] **Track color picker** - Easy color selection
- [ ] **Track icons** - Show synth type icon
- [ ] **Drag to reorder tracks**

---

## Phase 4: UI Polish - Synth Editors

### General Issues (all synths):
- Knobs are small
- Labels hard to read
- Sections not clearly separated
- Inconsistent layouts

### Improvements:
- [ ] **Consistent section headers** - Clear visual separation
- [ ] **Larger knobs** - 60px minimum, with value display
- [ ] **Better labels** - Readable font size, good contrast
- [ ] **Parameter value tooltips** - Show exact value on hover
- [ ] **Visual grouping** - Background colors for sections

### Per-Synth:
- [ ] **FM Synth** - Algorithm visualization, operator routing diagram
- [ ] **Poly Pad** - Waveform selector visual
- [ ] **Organ** - Drawbar visualization (vertical sliders)
- [ ] **String** - Section mix visualization
- [ ] **Pro Synth** - Wavetable display, mod matrix visualization
- [ ] **Sampler** - Waveform display, drag-drop zone
- [ ] **SoundFont** - Instrument browser, category icons
- [ ] **Drums** - Larger pads, visual feedback on hit

---

## Phase 5: UI Polish - Timeline

### Current Issues:
- Basic grid appearance
- Clips look plain
- No playhead glow

### Improvements:
- [ ] **Better grid lines** - Subtle but visible bar/beat lines
- [ ] **Improved clip appearance** - Rounded corners, gradients, shadows
- [ ] **Playhead styling** - Glowing line, visible at all zoom levels
- [ ] **Zoom controls** - Horizontal/vertical zoom buttons
- [ ] **Loop region styling** - Clear visual for loop start/end

---

## Phase 6: UI Polish - Mixer

### Current Issues:
- Channel strips narrow
- Meters basic
- No master section styling

### Improvements:
- [ ] **Wider channel strips** - More room for controls
- [ ] **Professional meters** - Segmented LED style, peak hold
- [ ] **Fader styling** - Better thumb, track appearance
- [ ] **Master section** - Distinct styling, larger meters
- [ ] **Effect slots** - Visual indication of active effects

---

## Phase 7: UI Polish - Piano Roll

### Improvements:
- [ ] **Note colors** - Velocity-based coloring
- [ ] **Grid styling** - Clear octave separation
- [ ] **Selection styling** - Clear selection highlight
- [ ] **Tool indicators** - Show current tool (draw/select/erase)
- [ ] **Velocity lane** - Better visualization

---

## Phase 8: Color Scheme Refinement

### Current Palette:
- bgPrimary: #1a1a1a (very dark)
- bgSecondary: #242424
- bgTertiary: #2d2d2d
- accentBlue: #3b82f6

### Proposed Improvements:
- [ ] **Slightly warmer darks** - Less pure black, more comfortable
- [ ] **Accent color variations** - Lighter/darker versions for states
- [ ] **Better contrast** - Ensure text is always readable
- [ ] **Consistent hover states** - Subtle highlight on interactive elements

---

## Phase 9: Typography & Spacing

### Improvements:
- [ ] **Define type scale** - Consistent font sizes (11, 13, 16, 20px)
- [ ] **Define spacing scale** - Consistent padding (4, 8, 12, 16, 24px)
- [ ] **Better font weights** - Use bold for headers, regular for labels
- [ ] **Monospace for values** - Numbers should align

---

## Phase 10: Missing Features

### High Priority:
- [ ] **Undo/Redo in menu** - Currently commented out
- [ ] **Select All (Cmd+A)** - Not implemented
- [ ] **Loop toggle (L key)** - Not implemented
- [ ] **Clip navigation ([/])** - Not implemented

### Medium Priority:
- [ ] **Keyboard shortcuts help** - Show available shortcuts
- [ ] **Preferences dialog** - Audio settings, appearance, shortcuts
- [ ] **About dialog** - Version, credits

---

## Phase 11: Sound Library

### Options:
1. **Bundle a free SoundFont** - FluidR3_GM (~150MB) or smaller alternative
2. **Download on first launch** - Fetch from server
3. **Point to system SoundFonts** - Let user select their own

### Recommendation:
- Bundle a smaller GM SoundFont (~30MB)
- Add "Download Full Library" option for FluidR3

---

## Implementation Priority

### Week 1: Critical Fixes
1. Project rename UI
2. Test & fix save/load
3. Fix preset system
4. Bundle basic SoundFont

### Week 2: Core UI Polish
1. Transport bar improvements
2. Track list improvements
3. Synth editor consistency

### Week 3: Advanced UI
1. Timeline polish
2. Mixer polish
3. Piano roll polish

### Week 4: Final Polish
1. Color scheme refinement
2. Typography & spacing
3. Missing features
4. Testing & bug fixes

---

## Quick Wins (Can do immediately)

1. **Larger knobs** - Change KNOB_SIZE constant
2. **Better section headers** - Add background color
3. **Button hover states** - Add subtle highlight
4. **Increase padding** - More breathing room
5. **Fix drum sounds** - Already done!

---

## Files to Modify

| Component | File |
|-----------|------|
| Colors/Theme | `Source/UI/LookAndFeel.cpp` |
| Transport Bar | `Source/UI/TransportBar.cpp` |
| Track List | `Source/UI/Tracks/TrackHeader.cpp` |
| Synth Editors | `Source/UI/Synths/*.cpp` |
| Timeline | `Source/UI/Timeline/*.cpp` |
| Mixer | `Source/UI/Mixer/*.cpp` |
| Piano Roll | `Source/UI/PianoRoll/*.cpp` |
| Main Layout | `Source/MainWindow.cpp` |
