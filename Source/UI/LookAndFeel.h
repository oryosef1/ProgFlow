#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Spacing constants - consistent spacing throughout the app
 */
namespace ProgFlowSpacing
{
    static constexpr int XS = 4;
    static constexpr int SM = 8;
    static constexpr int MD = 12;
    static constexpr int LG = 16;
    static constexpr int XL = 24;

    // Component sizes - Saturn design
    static constexpr int KNOB_SIZE = 48;           // Saturn knob diameter
    static constexpr int KNOB_WITH_LABEL = 80;     // Knob + gap + label + value
    static constexpr int SECTION_HEADER_HEIGHT = 20;
    static constexpr int COMBO_HEIGHT = 28;
    static constexpr int DIVIDER_WIDTH = 1;
    static constexpr int RESIZE_HANDLE = 4;        // Drag handle thickness

    // Card panel (Saturn design)
    static constexpr int CARD_CORNER_RADIUS = 6;
    static constexpr int CARD_PADDING = 12;

    // Glass panel
    static constexpr int GLASS_CORNER_RADIUS = 8;
    static constexpr int GLASS_BORDER_WIDTH = 1;

    // Header
    static constexpr int HEADER_HEIGHT = 44;

    // Legacy (keep for compatibility)
    static constexpr int KNOB_MIN_SIZE = 48;
    static constexpr int KNOB_PREFERRED = 48;
    static constexpr int SECTION_CORNER_RADIUS = 6;
    static constexpr int BUTTON_CORNER_RADIUS = 4;
}

/**
 * ColorScheme - Holds all colors for a theme
 * Modern design with glassmorphism, glows, and depth
 */
struct ColorScheme
{
    // Backgrounds (with depth)
    juce::Colour bgPrimary;      // Deep background
    juce::Colour bgSecondary;    // Panel backgrounds
    juce::Colour bgTertiary;     // Hover states
    juce::Colour bgHover;
    juce::Colour sectionBg;      // Section backgrounds
    juce::Colour surfaceBg;      // Raised surfaces
    juce::Colour dividerLine;    // Thin section dividers

    // Glass effect
    juce::Colour glassOverlay;   // Frosted glass overlay
    juce::Colour glassHover;     // Glass hover state
    juce::Colour glassBorder;    // Subtle glass border

    // Accents (vibrant)
    juce::Colour accentBlue;
    juce::Colour accentGreen;
    juce::Colour accentOrange;
    juce::Colour accentRed;

    // Glow variants (for bloom effects)
    juce::Colour glowBlue;
    juce::Colour glowGreen;
    juce::Colour glowOrange;
    juce::Colour glowRed;

    // Knob colors
    juce::Colour knobBody;       // Knob background
    juce::Colour knobBodyLight;  // Knob gradient light
    juce::Colour knobArcBg;      // Inactive arc
    juce::Colour knobIndicator;  // Position indicator

    // Text
    juce::Colour textPrimary;
    juce::Colour textSecondary;
    juce::Colour textMuted;      // Section headers
    juce::Colour textDisabled;

    // Borders
    juce::Colour border;
    juce::Colour borderLight;
    juce::Colour borderGlow;     // Glowing border on focus

    // Meters
    juce::Colour meterGreen;
    juce::Colour meterYellow;
    juce::Colour meterRed;
    juce::Colour meterBg;
};

/**
 * ThemeManager - Singleton that manages the current color scheme
 */
class ThemeManager
{
public:
    enum class Theme { Dark, Light };

    static ThemeManager& getInstance();

    void setTheme(Theme theme);
    Theme getTheme() const { return currentTheme; }

    const ColorScheme& getColors() const { return *currentColors; }

    // Listener for theme changes
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void themeChanged() = 0;
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    ThemeManager();

    Theme currentTheme = Theme::Dark;
    const ColorScheme* currentColors;

    ColorScheme darkScheme;
    ColorScheme lightScheme;

    juce::ListenerList<Listener> listeners;
};

/**
 * ProgFlow Color Palette - Global accessor functions
 * Returns colors from the current theme
 */
namespace ProgFlowColours
{
    // Backgrounds
    inline juce::Colour bgPrimary()    { return ThemeManager::getInstance().getColors().bgPrimary; }
    inline juce::Colour bgSecondary()  { return ThemeManager::getInstance().getColors().bgSecondary; }
    inline juce::Colour bgTertiary()   { return ThemeManager::getInstance().getColors().bgTertiary; }
    inline juce::Colour bgHover()      { return ThemeManager::getInstance().getColors().bgHover; }
    inline juce::Colour sectionBg()    { return ThemeManager::getInstance().getColors().sectionBg; }
    inline juce::Colour surfaceBg()    { return ThemeManager::getInstance().getColors().surfaceBg; }
    inline juce::Colour dividerLine()  { return ThemeManager::getInstance().getColors().dividerLine; }

    // Glass effect
    inline juce::Colour glassOverlay() { return ThemeManager::getInstance().getColors().glassOverlay; }
    inline juce::Colour glassHover()   { return ThemeManager::getInstance().getColors().glassHover; }
    inline juce::Colour glassBorder()  { return ThemeManager::getInstance().getColors().glassBorder; }

    // Accents
    inline juce::Colour accentBlue()   { return ThemeManager::getInstance().getColors().accentBlue; }
    inline juce::Colour accentGreen()  { return ThemeManager::getInstance().getColors().accentGreen; }
    inline juce::Colour accentOrange() { return ThemeManager::getInstance().getColors().accentOrange; }
    inline juce::Colour accentRed()    { return ThemeManager::getInstance().getColors().accentRed; }

    // Glows
    inline juce::Colour glowBlue()     { return ThemeManager::getInstance().getColors().glowBlue; }
    inline juce::Colour glowGreen()    { return ThemeManager::getInstance().getColors().glowGreen; }
    inline juce::Colour glowOrange()   { return ThemeManager::getInstance().getColors().glowOrange; }
    inline juce::Colour glowRed()      { return ThemeManager::getInstance().getColors().glowRed; }

    // Knobs
    inline juce::Colour knobBody()     { return ThemeManager::getInstance().getColors().knobBody; }
    inline juce::Colour knobBodyLight(){ return ThemeManager::getInstance().getColors().knobBodyLight; }
    inline juce::Colour knobArcBg()    { return ThemeManager::getInstance().getColors().knobArcBg; }
    inline juce::Colour knobIndicator(){ return ThemeManager::getInstance().getColors().knobIndicator; }

    // Text
    inline juce::Colour textPrimary()  { return ThemeManager::getInstance().getColors().textPrimary; }
    inline juce::Colour textSecondary(){ return ThemeManager::getInstance().getColors().textSecondary; }
    inline juce::Colour textMuted()    { return ThemeManager::getInstance().getColors().textMuted; }
    inline juce::Colour textDisabled() { return ThemeManager::getInstance().getColors().textDisabled; }

    // Borders
    inline juce::Colour border()       { return ThemeManager::getInstance().getColors().border; }
    inline juce::Colour borderLight()  { return ThemeManager::getInstance().getColors().borderLight; }
    inline juce::Colour borderGlow()   { return ThemeManager::getInstance().getColors().borderGlow; }

    // Meters
    inline juce::Colour meterGreen()   { return ThemeManager::getInstance().getColors().meterGreen; }
    inline juce::Colour meterYellow()  { return ThemeManager::getInstance().getColors().meterYellow; }
    inline juce::Colour meterRed()     { return ThemeManager::getInstance().getColors().meterRed; }
    inline juce::Colour meterBg()      { return ThemeManager::getInstance().getColors().meterBg; }
}

/**
 * ProgFlowLookAndFeel - Custom styling for all JUCE components
 * Supports both dark and light themes
 */
class ProgFlowLookAndFeel : public juce::LookAndFeel_V4,
                            public ThemeManager::Listener
{
public:
    ProgFlowLookAndFeel();
    ~ProgFlowLookAndFeel() override;

    // ThemeManager::Listener
    void themeChanged() override;

    //==========================================================================
    // Button styling
    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;

    //==========================================================================
    // Rotary slider (knob) styling
    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider& slider) override;

    //==========================================================================
    // Linear slider styling
    void drawLinearSlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style,
                          juce::Slider& slider) override;

    //==========================================================================
    // Label styling
    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    //==========================================================================
    // ComboBox styling
    void drawComboBox(juce::Graphics& g, int width, int height,
                      bool isButtonDown, int buttonX, int buttonY,
                      int buttonW, int buttonH,
                      juce::ComboBox& box) override;

private:
    juce::Font defaultFont;
    void updateColours();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProgFlowLookAndFeel)
};
