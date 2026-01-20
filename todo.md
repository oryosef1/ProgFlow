# ProgFlow TODO

## In Progress
- [ ] UI Polish - was interrupted mid-work (see `docs/plans/2026-01-20-ui-redesign-implementation.md`)

## High Priority

### Critical Fixes
- [ ] Add project rename UI (click project name in transport bar)
- [ ] Test save/load (Cmd+S, Cmd+O)
- [ ] Fix "Untitled *" dirty state display
- [ ] Bundle default SoundFont (.sf2) for SoundFontPlayer
- [ ] Investigate preset loading issues

### Missing Keyboard Shortcuts
- [ ] Select All (Cmd+A)
- [ ] Loop toggle (L key)
- [ ] Clip navigation ([ and ] keys)

## UI Polish

### Transport Bar
- [ ] Larger, cleaner transport buttons
- [ ] Editable project name
- [ ] Better time display
- [ ] Improved meters with peak hold
- [ ] Tempo tap visual feedback

### Track List
- [ ] Larger track headers
- [ ] Better S/M/R buttons (Solo yellow, Mute red, Record red)
- [ ] Improved volume/pan controls
- [ ] Track color picker
- [ ] Track icons for synth type
- [ ] Drag to reorder tracks

### Synth Editors
- [ ] Consistent section headers
- [ ] Larger knobs (60px minimum)
- [ ] Better labels
- [ ] Parameter value tooltips
- [ ] Visual grouping with background colors

### Timeline
- [ ] Better grid lines
- [ ] Improved clip appearance (rounded, shadows)
- [ ] Playhead glow styling
- [ ] Zoom controls
- [ ] Loop region styling

### Mixer
- [ ] Wider channel strips
- [ ] Professional LED-style meters
- [ ] Better fader styling
- [ ] Distinct master section
- [ ] Effect slot indicators

### Piano Roll
- [ ] Velocity-based note colors
- [ ] Clear octave separation
- [ ] Selection highlight styling
- [ ] Tool indicators
- [ ] Better velocity lane

## Code Cleanup
- [ ] Remove DBG() statements from production code
- [ ] Fix manual FileChooser deletion in SoundFontPlayerEditor.cpp
- [ ] Standardize font sizes (11, 13, 16, 20px)
- [ ] Standardize spacing (4, 8, 12, 16, 24px)

## Completed
- [x] Drum kit switching (was only updating 4 pads)
- [x] Quiet drum sounds (Rim, Shaker, Cymbal, Clap)
