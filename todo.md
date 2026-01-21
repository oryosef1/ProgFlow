# ProgFlow TODO

## Current Focus: "Saturn" UI Redesign

FabFilter-inspired professional dark theme with purple accents. See `docs/plans/2026-01-20-saturn-ui-redesign.md` for full spec.

---

### Phase 1: Foundation ✓
- [x] **1.1** Update LookAndFeel with Saturn color palette
- [x] **1.2** Redesign RotaryKnob (48px, metallic gradient, value display)
- [x] **1.3** Create CardPanel component (gradient bg, shadow, rounded corners)
- [x] **1.4** Style ComboBox (purple focus, custom arrow)
- [x] **1.5** Style S/M/R buttons (cyan/gold/coral colors)

### Phase 2: Remove Redundant Synths ✓
- [x] **2.1** Remove PolyPadSynth (files + all references)
- [x] **2.2** Remove OrganSynth (files + all references)
- [x] **2.3** Remove StringSynth (files + all references)
- [x] **2.4** Update SynthType enum and SynthFactory

### Phase 3: FM Editor (Template) ✓
- [x] **3.1** Refactor FMSynthEditor with CardPanel layout
- [x] **3.2** Test and polish FM editor
- [x] **3.3** Add descriptive tooltips to all knobs
- [x] **3.4** Add resizable bottom panel with drag handle

### Phase 4: Other Synth Editors ✓
- [x] **4.1** AnalogSynthEditor redesign
- [x] **4.2** ProSynthEditor redesign
- [x] **4.3** DrumSynthEditor redesign
- [x] **4.4** SamplerEditor redesign
- [x] **4.5** SoundFontPlayerEditor redesign

### Phase 5: Main Panels ✓
- [x] **5.1** Track List panel redesign
- [x] **5.2** Transport bar redesign
- [x] **5.3** Mixer panel redesign
- [x] **5.4** Timeline/Arrangement polish

### Phase 6: Final Polish ✓
- [x] **6.1** Consistent spacing/alignment pass
- [x] **6.2** Hover/focus states everywhere
- [x] **6.3** Final testing (56,309 tests passed)

---

## Backlog (After UI Redesign)

### Critical Fixes
- [x] Add project rename UI (click project name to rename)
- [x] Fix "Untitled *" dirty state display
- [x] Fix record button (arms tracks + starts playback with count-in)
- [ ] Bundle default SoundFont (.sf2)

### Missing Keyboard Shortcuts
- [x] Select All (Cmd+A) - already implemented
- [x] Loop toggle (L key) - already implemented
- [x] Clip navigation ([ and ] keys) - already implemented

### Code Cleanup
- [x] Remove DBG() statements from main files

---

## Completed
- [x] Drum kit switching fix
- [x] Quiet drum sounds fix
- [x] Git repository initialized
- [x] CLAUDE.md created
- [x] Initial UI redesign plan (ProgFlow Modern) - superseded by Saturn
- [x] Saturn UI redesign plan finalized
- [x] **Phase 1: Foundation** - Saturn colors, knob redesign, CardPanel, ComboBox styling, S/M/R buttons
- [x] **Phase 2: Remove Synths** - Removed PolyPad, Organ, String synths (now 6 synths total)
- [x] **Phase 3: FM Editor** - Refactored with CardPanel layout, tooltips, resizable panel
- [x] **Phase 4: Other Synth Editors** - Redesigned Analog, Pro, Drum, Sampler, SoundFont editors with CardPanel layout and tooltips
- [x] **Phase 5: Main Panels** - Redesigned Track List, Transport bar, Mixer, Timeline with Saturn styling
- [x] **Phase 6: Final Polish** - Consistent spacing, hover/focus states, 56,309 tests passing
