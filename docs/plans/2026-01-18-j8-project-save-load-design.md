# Phase J8: Project Save/Load & Audio Export Design

**Date:** 2026-01-18
**Status:** Approved

## Overview

Implement project file persistence and audio export for the JUCE version of ProgFlow. Maintains full compatibility with Electron `.progflow` files while adding JUCE-specific extensions for plugin state.

## Key Decisions

1. **Full compatibility** with Electron `.progflow` JSON format
2. **Base64 inline** for plugin state (single file, easy to share)
3. **JUCE PropertiesFile** for app settings and recent projects
4. **WAV + MP3 export** using LAME for MP3 encoding

---

## Project File Format

**File Extension:** `.progflow`

**JSON Structure (Version 2):**
```json
{
  "version": 2,
  "name": "My Song",
  "bpm": 120,
  "timeSignature": [4, 4],
  "tracks": [...],
  "markers": [...]
}
```

Version 1 = Electron format (no plugins), Version 2 = JUCE extensions.

### Track Schema

```json
{
  "id": "uuid-string",
  "name": "Synth 1",
  "color": "#3b82f6",
  "volume": 0.8,
  "pan": 0.0,
  "muted": false,
  "soloed": false,
  "synthType": "analog",
  "synthParams": { "preset": "init", ... },
  "clips": [...],
  "pluginInstrument": null,
  "pluginEffects": []
}
```

### Clip Schema

```json
{
  "id": "uuid-string",
  "name": "Clip",
  "color": "#3b82f6",
  "startBar": 0.0,
  "durationBars": 4.0,
  "notes": [
    { "id": "uuid", "midiNote": 60, "startBeat": 0.0, "durationBeats": 1.0, "velocity": 0.8 }
  ]
}
```

### Plugin Schema (JUCE-specific)

```json
{
  "name": "Serum",
  "manufacturer": "Xfer Records",
  "format": "VST3",
  "uid": "plugin-unique-id",
  "state": "base64-encoded-binary..."
}
```

---

## C++ Implementation

### New Files

```
Source/Project/
├── ProjectManager.h/cpp      # Save/load/recent files/autosave
├── ProjectSerializer.h/cpp   # JSON serialization logic
└── ExportDialog.h/cpp        # Export format selection UI
```

### ProjectSerializer

Handles conversion between JUCE objects and JSON:

- `juce::var serialize(AudioEngine&)` - Objects → JSON
- `bool deserialize(const juce::var&, AudioEngine&)` - JSON → Objects
- Per-type methods: `serializeTrack()`, `serializeClip()`, `serializeNote()`, `serializePlugin()`
- Base64 helpers for plugin state encoding/decoding

### ProjectManager

Handles file operations and application state:

- `saveProject()` / `saveProjectAs()` - Native file dialogs
- `openProject()` / `openRecentProject(path)`
- `newProject()` - Reset to empty state
- Recent files using `juce::RecentlyOpenedFilesList`
- Autosave timer with crash recovery
- Dirty flag tracking for unsaved changes prompts

### Integration Points

- `MainWindow` File menu: New, Open, Open Recent, Save, Save As, Export
- Keyboard shortcuts: Cmd+N, Cmd+O, Cmd+S, Cmd+Shift+S, Cmd+E
- Window title: project name + dirty indicator (*)
- Close confirmation dialog when unsaved changes exist

---

## Audio Export

### Formats

| Format | Spec | Use Case |
|--------|------|----------|
| WAV | 44.1kHz, 16-bit stereo | Lossless master |
| MP3 | 128/192/320 kbps | Sharing, streaming |

### Offline Rendering

Uses existing AudioEngine, processes faster than realtime:

```cpp
double projectLengthSeconds = calculateProjectLength();
int totalSamples = sampleRate * projectLengthSeconds;
AudioBuffer<float> offlineBuffer(2, totalSamples);

for (int pos = 0; pos < totalSamples; pos += blockSize) {
    engine.processBlock(offlineBuffer, pos, blockSize);
}
```

### MP3 Encoding

Uses libmp3lame (LAME) for MP3 encoding:

```cpp
lame_t lame = lame_init();
lame_set_in_samplerate(lame, 44100);
lame_set_num_channels(lame, 2);
lame_set_brate(lame, 128);
lame_init_params(lame);

lame_encode_buffer_interleaved_ieee_float(...);
lame_encode_flush(...);
```

### Export Dialog

- Format selector (WAV / MP3)
- Quality options (bitrate for MP3)
- Progress bar during render
- Cancel button

---

## Autosave & Recovery

### Autosave Location

`~/Library/Application Support/ProgFlow/autosave/`

### Behavior

1. Timer fires every 2 minutes (configurable)
2. Only saves if project is dirty AND has been saved once
3. Writes to `autosave/{project-name}-recovery.progflow`
4. On clean exit, deletes autosave file
5. On startup, checks for recovery files and prompts user

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Cmd+N | New Project |
| Cmd+O | Open Project |
| Cmd+S | Save |
| Cmd+Shift+S | Save As |
| Cmd+E | Export Audio |

---

## Implementation Order

1. ProjectSerializer (JSON read/write)
2. Track/Clip `toVar()`/`fromVar()` methods
3. ProjectManager (save/open dialogs)
4. File menu + shortcuts in MainWindow
5. Recent files list
6. Autosave system
7. WAV export (offline render)
8. MP3 export (LAME integration)
9. Export dialog UI

---

## Files to Modify

- `CMakeLists.txt` - LAME dependency, new source files
- `MainWindow.h/cpp` - File menu, shortcuts, title bar
- `Track.h/cpp` - Serialization methods
- `MidiClip.h/cpp` - Already has `toVar()`/`fromVar()`
- `AudioEngine.h/cpp` - Offline render method
