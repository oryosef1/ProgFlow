# Claude Instructions for ProgFlow

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
│   └── Synths/           # Synth editors
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

## UI Constants

Located in `Source/UI/LookAndFeel.h`:
- `ProgFlowSpacing` namespace for consistent spacing
- `ColorScheme` struct for theme colors
- Standard knob size, button heights, etc.

## Testing

Tests are in `Tests/` directory:
- Unit tests for DSP, clips, automation
- Integration tests for full workflow
- Stress tests for performance

## Current Status

See `docs/IMPROVEMENT_PLAN.md` for known issues and planned improvements.
See `docs/plans/` for detailed design documents.
