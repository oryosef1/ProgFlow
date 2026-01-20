# ProgFlow

A professional desktop DAW (Digital Audio Workstation) built with JUCE framework.

## Features

### Audio Engine
- Multi-track arrangement (50+ tracks tested)
- Real-time audio processing
- Sample-accurate playback
- Master effects chain

### Instruments (9 types)
- **AnalogSynth** - Classic 2-oscillator subtractive synthesis
- **FMSynth** - Frequency modulation synthesis
- **PolyPadSynth** - Lush pad sounds with unison
- **OrganSynth** - Tonewheel organ emulation
- **StringSynth** - Physical modeling strings
- **ProSynth** - Advanced wavetable with 210 presets, mod matrix, LFOs
- **Sampler** - Sample playback with time-stretch
- **SoundFontPlayer** - 128 GM instrument presets
- **DrumSynth** - 5 drum kits, 16 pads, choke groups

### Effects (16 types)
Reverb, Delay, Chorus, Phaser, Flanger, Tremolo, Distortion, Bitcrusher, Filter, EQ, Compressor, Sidechain Compressor, Gate, Limiter, Amp Simulator, Cabinet

### Editing
- Timeline/arrangement view
- Piano roll with velocity editing
- Clip drag, resize, split
- Quantize with strength control
- Ghost notes
- Automation lanes (Write/Touch/Latch modes)

### Mixing
- Channel strips with VU meters
- Volume, pan, mute, solo
- Per-track effect chains
- Master bus processing

### Project
- Save/load projects (JSON format)
- Export to WAV/MP3
- Undo/redo system
- Keyboard shortcuts

## Building

### Prerequisites
- CMake 3.22+
- C++17 compiler (Clang, GCC, MSVC)
- macOS 10.15+ / Windows 10+ / Linux

### Build Commands

**macOS:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
open build/ProgFlow_artefacts/Release/ProgFlow.app
```

**Windows:**
```bash
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

**Linux:**
```bash
sudo apt-get install libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype6-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Running Tests
```bash
./build/ProgFlowTests_artefacts/Release/ProgFlowTests
```

## Plugin Formats

ProgFlow can also be built as a VST3/AU plugin:
- `ProgFlow.vst3` - VST3 plugin
- `ProgFlow.component` - Audio Unit (macOS)

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Space | Play/Pause |
| Enter | Stop & return to start |
| R | Toggle record arm |
| T | Tap tempo |
| S | Toggle snap |
| K | Toggle virtual keyboard |
| G | Toggle ghost notes |
| Tab | Toggle mixer view |
| Cmd/Ctrl+Z | Undo |
| Cmd/Ctrl+Shift+Z | Redo |
| Cmd/Ctrl+S | Save project |
| Cmd/Ctrl+O | Open project |
| Cmd/Ctrl+E | Split clip at playhead |
| Delete | Delete selected |

## Architecture

```
Source/
├── Main.cpp              # Application entry point
├── MainWindow.h/cpp      # Main window and layout
├── Audio/                # Audio engine
│   ├── AudioEngine       # Core audio processor
│   ├── Track             # Track management
│   ├── MidiClip          # MIDI note data
│   ├── AudioClip         # Audio sample data
│   ├── Synths/           # 9 instrument implementations
│   └── Effects/          # 16 effect implementations
├── UI/                   # User interface
│   ├── LookAndFeel       # Theming (dark/light)
│   ├── TransportBar      # Play/stop/tempo controls
│   ├── Timeline/         # Arrangement view
│   ├── PianoRoll/        # Note editor
│   ├── Mixer/            # Channel strips
│   └── Synths/           # Instrument editors
├── Project/              # File I/O
│   ├── ProjectManager    # Save/load orchestration
│   ├── ProjectSerializer # JSON serialization
│   └── AudioExporter     # WAV/MP3 export
└── Core/                 # Utilities
    └── UndoManager       # Undo/redo system
```

## Dependencies

- **JUCE 8.0.4** - Audio application framework
- **RubberBand 3.3.0** - Time-stretching library
- **TinySoundFont** - SoundFont (SF2) playback

## License

[Add license information]
