# ProgFlow TODO

## Current Focus: "Saturn" UI Redesign

FabFilter-inspired professional dark theme with purple accents. See `docs/plans/2026-01-20-saturn-ui-redesign.md` for full spec.

---

### Phase 1: Foundation
- [ ] **1.1** Update LookAndFeel with Saturn color palette
- [ ] **1.2** Redesign RotaryKnob (48px, metallic gradient, value display)
- [ ] **1.3** Create CardPanel component (gradient bg, shadow, rounded corners)
- [ ] **1.4** Style ComboBox (purple focus, custom arrow)
- [ ] **1.5** Style S/M/R buttons (cyan/gold/coral colors)

### Phase 2: Remove Redundant Synths
- [ ] **2.1** Remove PolyPadSynth (files + all references)
- [ ] **2.2** Remove OrganSynth (files + all references)
- [ ] **2.3** Remove StringSynth (files + all references)
- [ ] **2.4** Update SynthType enum and SynthFactory

### Phase 3: FM Editor (Template)
- [ ] **3.1** Refactor FMSynthEditor with CardPanel layout
- [ ] **3.2** Test and polish FM editor

### Phase 4: Other Synth Editors
- [ ] **4.1** AnalogSynthEditor redesign
- [ ] **4.2** ProSynthEditor redesign
- [ ] **4.3** DrumSynthEditor redesign
- [ ] **4.4** SamplerEditor redesign
- [ ] **4.5** SoundFontPlayerEditor redesign

### Phase 5: Main Panels
- [ ] **5.1** Track List panel redesign
- [ ] **5.2** Transport bar redesign
- [ ] **5.3** Mixer panel redesign
- [ ] **5.4** Timeline/Arrangement polish

### Phase 6: Final Polish
- [ ] **6.1** Consistent spacing/alignment pass
- [ ] **6.2** Hover/focus states everywhere
- [ ] **6.3** Final testing

---

## Backlog (After UI Redesign)

### Critical Fixes
- [ ] Add project rename UI
- [ ] Test save/load (Cmd+S, Cmd+O)
- [ ] Fix "Untitled *" dirty state display
- [ ] Bundle default SoundFont (.sf2)

### Missing Keyboard Shortcuts
- [ ] Select All (Cmd+A)
- [ ] Loop toggle (L key)
- [ ] Clip navigation ([ and ] keys)

### Code Cleanup
- [ ] Remove DBG() statements from production code

---

## Completed
- [x] Drum kit switching fix
- [x] Quiet drum sounds fix
- [x] Git repository initialized
- [x] CLAUDE.md created
- [x] Initial UI redesign plan (ProgFlow Modern) - superseded by Saturn
- [x] Saturn UI redesign plan finalized
