# Claude Instructions for ProgFlow

## Workflow Rules

### IMPORTANT: Always Update todo.md
After completing ANY task:
1. Mark the task as complete in `todo.md`
2. Add any new tasks discovered during implementation
3. Update "In Progress" section if moving to next task

This keeps the project state clear between sessions.

### Before Starting Work
1. Read `todo.md` to understand current progress
2. Read relevant design docs in `docs/plans/`
3. Check git status for any uncommitted work

---

## Project Overview

ProgFlow is a professional desktop DAW (Digital Audio Workstation) built with the JUCE 8.0.4 framework. It supports multi-track arrangement, 9 built-in synthesizers, 16 audio effects, and can run as standalone or VST3/AU plugin.

## Technology Stack

- **Language**: C++20
- **Framework**: JUCE 8.0.4
- **Build System**: CMake 3.22+
- **Dependencies**: RubberBand 3.3.0 (time-stretch), TinySoundFont (SF2 playback)
- **Platforms**: macOS, Windows, Linux

## Code Style

- Use PascalCase for classes and methods
- Use camelCase for variables and parameters
- Member variables: no prefix (JUCE convention)
- One class per file, filename matches class name
- Use JUCE idioms: `std::unique_ptr`, `juce::String`, `juce::Array`
- Prefer JUCE types over STL where equivalent exists

## Architecture

```
Source/
├── Main.cpp              # Application entry point
├── MainWindow.cpp/h      # Main window and layout
├── Audio/                # Audio engine and DSP
│   ├── AudioEngine       # Core audio processor
│   ├── Track             # Track management
│   ├── MidiClip          # MIDI data
│   ├── AudioClip         # Audio samples
│   ├── Synths/           # 9 instrument types
│   └── Effects/          # 16 effect types
├── UI/                   # User interface
│   ├── LookAndFeel       # Theming system
│   ├── TransportBar      # Transport controls
│   ├── Timeline/         # Arrangement view
│   ├── PianoRoll/        # MIDI editor
│   ├── Mixer/            # Channel strips
│   ├── Synths/           # Synth editors
│   └── Common/           # Shared UI components (ModernKnob, GlassPanel, etc.)
├── Project/              # File I/O
│   ├── ProjectManager    # Save/load
│   ├── ProjectSerializer # JSON format
│   └── AudioExporter     # WAV/MP3 export
├── Core/                 # Utilities
│   └── UndoManager       # Undo/redo
└── MIDI/
    └── MidiLearnManager  # MIDI learn
```

## Build Commands

```bash
# macOS Release
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run app
open build/ProgFlow_artefacts/Release/ProgFlow.app

# Run tests
./build/ProgFlowTests_artefacts/Release/ProgFlowTests
```

## Key Files

| Component | Location |
|-----------|----------|
| Theme/Colors | `Source/UI/LookAndFeel.cpp` |
| Main Layout | `Source/MainWindow.cpp` |
| Audio Engine | `Source/Audio/AudioEngine.cpp` |
| Transport | `Source/UI/TransportBar.cpp` |
| Timeline | `Source/UI/Timeline/TimelinePanel.cpp` |
| Piano Roll | `Source/UI/PianoRoll/PianoRollPanel.cpp` |
| Mixer | `Source/UI/Mixer/MixerPanel.cpp` |

## UI Design System (ProgFlow Modern)

See `docs/plans/2026-01-20-ui-modern-design.md` for full design spec.

### Colors (Dark Theme)
```cpp
bgPrimary      = #0d1117   // Deep background
bgSecondary    = #161b22   // Panel background
bgGlass        = #ffffff10 // Frosted glass overlay
accentBlue     = #4C9EFF   // Primary accent
accentGreen    = #3DDC84   // Play, positive
accentOrange   = #FFAB40   // Warnings
accentRed      = #FF5252   // Record, errors
```

### Component Sizes
```cpp
KNOB_SIZE = 52            // Modern knob diameter
GLASS_CORNER_RADIUS = 8   // Rounded panel corners
RESIZE_HANDLE = 4         // Drag handle thickness
```

## Testing

Tests are in `Tests/` directory:
- Unit tests for DSP, clips, automation
- Integration tests for full workflow
- Stress tests for performance

Run tests: `./build/ProgFlowTests_artefacts/Release/ProgFlowTests`

## Current Status

See `todo.md` for current progress and next tasks.
See `docs/plans/` for detailed design documents.
