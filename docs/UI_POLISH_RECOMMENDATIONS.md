# UI Polish Recommendations for ProgFlow

## Summary

Overall the UI is well-structured with a consistent theming system. Below are recommendations for professional polish.

## Completed Fixes

- [x] Solo button now uses `ProgFlowColours::accentOrange()` instead of hardcoded yellow (ChannelStrip.cpp:233)

## High Priority

### 1. Remove Debug Statements from Production

**Files affected:**
- `TransportBar.cpp:355` - Tap tempo DBG
- `ToastManager.cpp:38` - Toast shown DBG
- `TimelinePanel.cpp:237, 809, 822` - Various DBG statements
- `MainWindow.cpp:363, 404, 467, 488, 901` - Multiple DBG statements

**Recommendation:** Wrap all DBG statements in `#if JUCE_DEBUG`:
```cpp
#if JUCE_DEBUG
    DBG("Tap tempo: " << bpm << " BPM");
#endif
```

### 2. Implement Missing Keyboard Shortcuts

| Shortcut | Feature | File | Line |
|----------|---------|------|------|
| Cmd+A | Select All | MainWindow.cpp | 363 |
| L | Loop Toggle | MainWindow.cpp | 403 |
| [ / ] | Clip Navigation | TimelinePanel.cpp | 167, 173 |

### 3. Add Undo/Redo to Edit Menu

Currently commented out at `MainWindow.cpp:952-955`. The undo system works via keyboard shortcuts but should be accessible from the menu.

## Medium Priority

### 4. Extract Layout Constants

Several files use hardcoded layout values. Extract to class constants for maintainability:

```cpp
// In header file
static constexpr int KNOB_SIZE = 50;
static constexpr int LABEL_HEIGHT = 16;
static constexpr int BUTTON_HEIGHT = 28;
static constexpr int HEADER_HEIGHT = 40;
```

**Files to update:**
- `DrumSynthEditor.cpp` (lines 206-226)
- `ProSynthEditor.cpp` (lines 284-308)
- `SoundFontPlayerEditor.cpp` (lines 256-278)

### 5. FileChooser Memory Management

In `SoundFontPlayerEditor.cpp:195-216`, the FileChooser is manually deleted in an async callback. Consider using `std::unique_ptr` for safer memory management:

```cpp
auto chooser = std::make_unique<juce::FileChooser>(...);
auto* chooserPtr = chooser.get();
chooserPtr->launchAsync(..., [chooser = std::move(chooser)](const auto& result) {
    // chooser automatically deleted when lambda is destroyed
});
```

## Low Priority

### 6. Consistent Spacing System

Consider defining a spacing scale for consistent padding/margins:

```cpp
namespace Spacing {
    constexpr int XS = 2;
    constexpr int SM = 4;
    constexpr int MD = 8;
    constexpr int LG = 16;
    constexpr int XL = 24;
}
```

### 7. Font Consistency

Some components use:
- `juce::Font(11.0f)`
- `juce::Font(12.0f)`
- `juce::Font(13.0f)`
- `juce::Font(14.0f)`
- `juce::Font(16.0f)`

Consider defining a type scale:

```cpp
namespace Typography {
    inline juce::Font caption() { return juce::Font(11.0f); }
    inline juce::Font body() { return juce::Font(13.0f); }
    inline juce::Font title() { return juce::Font(16.0f); }
    inline juce::Font mono() { return juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain); }
}
```

### 8. Animation/Transitions

For extra polish, consider adding subtle animations:
- Button hover fade transitions
- Panel slide-in animations
- Meter smoothing (already implemented in some places)

## Theme System

The existing `ThemeManager` and `ProgFlowColours` namespace is well-designed:

**Current color palette (Dark theme):**
- `bgPrimary`: #1a1a1a
- `bgSecondary`: #242424
- `bgTertiary`: #2d2d2d
- `accentBlue`: #3b82f6
- `accentGreen`: #10b981
- `accentOrange`: #f59e0b
- `accentRed`: #ef4444

**Suggestions:**
- Colors are good, modern dark theme
- Light theme is defined but may need testing
- Consider adding hover/active variants for interactive elements

## Testing Checklist

When making UI changes, verify:

- [ ] All themes look correct (dark/light)
- [ ] Keyboard navigation works
- [ ] Screen reader accessibility (JUCE Accessibility API)
- [ ] Different window sizes/resolutions
- [ ] High DPI displays
- [ ] Tooltips are helpful and present
