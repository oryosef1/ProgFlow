# ProgFlow TODO

## Current Focus: UI Redesign "ProgFlow Modern"

### Phase 1: Synth Editor Panel (In Progress)
- [ ] **1.1 Update LookAndFeel** - New color palette, gradients, glow effects
- [ ] **1.2 Create ModernKnob** - 52px, gradient body, glowing arc with bloom
- [ ] **1.3 Create GlassPanel** - Frosted glass effect, soft borders, rounded corners
- [ ] **1.4 Create EnvelopeVisualizer** - Mini ADSR curve display (80x40px)
- [ ] **1.5 Create ResizablePanel** - Drag-to-resize with min/max constraints
- [ ] **1.6 Refactor FMSynthEditor** - Apply new components and layout
- [ ] **1.7 Test and polish** - Verify looks good, smooth animations

### Phase 2: Other Synth Editors
- [ ] AnalogSynthEditor redesign
- [ ] PolyPadSynthEditor redesign
- [ ] OrganSynthEditor redesign
- [ ] StringSynthEditor redesign
- [ ] ProSynthEditor redesign
- [ ] DrumSynthEditor redesign
- [ ] SamplerEditor redesign
- [ ] SoundFontPlayerEditor redesign

### Phase 3: Track List Panel
- [ ] Make panel resizable (drag right edge)
- [ ] Larger track headers with modern styling
- [ ] Better S/M/R buttons (color-coded)
- [ ] Improved volume/pan controls
- [ ] Track color picker
- [ ] Track icons for synth type

### Phase 4: Transport Bar
- [ ] Modern styling with glass effect
- [ ] Larger, cleaner transport buttons
- [ ] Editable project name
- [ ] Better time display (monospace)
- [ ] Improved meters with peak hold

### Phase 5: Timeline/Arrangement
- [ ] Make panel resizable
- [ ] Better grid lines
- [ ] Improved clip appearance (rounded, shadows, glass)
- [ ] Playhead glow styling
- [ ] Zoom controls
- [ ] Loop region styling

### Phase 6: Mixer Panel
- [ ] Make panel resizable
- [ ] Wider channel strips with modern styling
- [ ] Professional LED-style meters
- [ ] Better fader styling
- [ ] Distinct master section

### Phase 7: Piano Roll
- [ ] Make panel resizable
- [ ] Velocity-based note colors
- [ ] Clear octave separation
- [ ] Modern toolbar styling
- [ ] Better velocity lane

---

## Backlog (After UI Redesign)

### Critical Fixes
- [ ] Add project rename UI
- [ ] Test save/load (Cmd+S, Cmd+O)
- [ ] Fix "Untitled *" dirty state display
- [ ] Bundle default SoundFont (.sf2)
- [ ] Investigate preset loading issues

### Missing Keyboard Shortcuts
- [ ] Select All (Cmd+A)
- [ ] Loop toggle (L key)
- [ ] Clip navigation ([ and ] keys)

### Code Cleanup
- [ ] Remove DBG() statements from production code
- [ ] Fix manual FileChooser deletion in SoundFontPlayerEditor.cpp

---

## Completed
- [x] Drum kit switching (was only updating 4 pads)
- [x] Quiet drum sounds (Rim, Shaker, Cymbal, Clap)
- [x] Git repository initialized
- [x] CLAUDE.md created
- [x] UI redesign plan finalized (ProgFlow Modern style)
