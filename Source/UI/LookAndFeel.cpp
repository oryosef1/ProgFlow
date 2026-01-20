#include "LookAndFeel.h"

//==============================================================================
// ThemeManager Implementation

ThemeManager::ThemeManager()
{
    // Dark theme (default) - Modern design with depth and glow
    // Backgrounds (subtle blue-black for depth)
    darkScheme.bgPrimary     = juce::Colour(0xff0d1117);  // Deep background
    darkScheme.bgSecondary   = juce::Colour(0xff161b22);  // Panel backgrounds
    darkScheme.bgTertiary    = juce::Colour(0xff1f2428);  // Hover states
    darkScheme.bgHover       = juce::Colour(0xff2d333b);
    darkScheme.sectionBg     = juce::Colour(0xff21262d);  // Section backgrounds
    darkScheme.surfaceBg     = juce::Colour(0xff1c2128);  // Raised surfaces
    darkScheme.dividerLine   = juce::Colour(0xff30363d);  // Thin dividers

    // Glass effect
    darkScheme.glassOverlay  = juce::Colour(0x18ffffff);  // 10% white
    darkScheme.glassHover    = juce::Colour(0x22ffffff);  // 13% white
    darkScheme.glassBorder   = juce::Colour(0x20ffffff);  // Subtle border

    // Accents (vibrant)
    darkScheme.accentBlue    = juce::Colour(0xff4C9EFF);  // Electric blue
    darkScheme.accentGreen   = juce::Colour(0xff3DDC84);  // Vibrant green
    darkScheme.accentOrange  = juce::Colour(0xffFFAB40);  // Warm orange
    darkScheme.accentRed     = juce::Colour(0xffFF5252);  // Bright red

    // Glow variants (25% opacity for bloom)
    darkScheme.glowBlue      = juce::Colour(0x404C9EFF);
    darkScheme.glowGreen     = juce::Colour(0x403DDC84);
    darkScheme.glowOrange    = juce::Colour(0x40FFAB40);
    darkScheme.glowRed       = juce::Colour(0x40FF5252);

    // Knob colors
    darkScheme.knobBody      = juce::Colour(0xff2a2f38);  // Knob background
    darkScheme.knobBodyLight = juce::Colour(0xff3a4048);  // Gradient light
    darkScheme.knobArcBg     = juce::Colour(0xff3a3f48);  // Inactive arc
    darkScheme.knobIndicator = juce::Colour(0xffffffff);  // Position indicator

    // Text
    darkScheme.textPrimary   = juce::Colour(0xffffffff);
    darkScheme.textSecondary = juce::Colour(0xffb0b0b0);
    darkScheme.textMuted     = juce::Colour(0xff6e7681);  // Section headers
    darkScheme.textDisabled  = juce::Colour(0xff484f58);

    // Borders
    darkScheme.border        = juce::Colour(0xff30363d);
    darkScheme.borderLight   = juce::Colour(0xff3a4048);
    darkScheme.borderGlow    = juce::Colour(0x404C9EFF);  // Glowing border

    // Meters
    darkScheme.meterGreen    = juce::Colour(0xff3DDC84);
    darkScheme.meterYellow   = juce::Colour(0xffFFAB40);
    darkScheme.meterRed      = juce::Colour(0xffFF5252);
    darkScheme.meterBg       = juce::Colour(0xff0d1117);

    // Light theme (updated to match modern style)
    lightScheme.bgPrimary     = juce::Colour(0xfff6f8fa);
    lightScheme.bgSecondary   = juce::Colour(0xffffffff);
    lightScheme.bgTertiary    = juce::Colour(0xffeaeef2);
    lightScheme.bgHover       = juce::Colour(0xffd0d7de);
    lightScheme.sectionBg     = juce::Colour(0xfff0f3f6);
    lightScheme.surfaceBg     = juce::Colour(0xfffafbfc);
    lightScheme.dividerLine   = juce::Colour(0xffd0d7de);

    // Glass effect (light)
    lightScheme.glassOverlay  = juce::Colour(0x10000000);
    lightScheme.glassHover    = juce::Colour(0x18000000);
    lightScheme.glassBorder   = juce::Colour(0x15000000);

    // Accents
    lightScheme.accentBlue    = juce::Colour(0xff0969da);
    lightScheme.accentGreen   = juce::Colour(0xff1a7f37);
    lightScheme.accentOrange  = juce::Colour(0xffbf8700);
    lightScheme.accentRed     = juce::Colour(0xffcf222e);

    // Glows (light theme)
    lightScheme.glowBlue      = juce::Colour(0x300969da);
    lightScheme.glowGreen     = juce::Colour(0x301a7f37);
    lightScheme.glowOrange    = juce::Colour(0x30bf8700);
    lightScheme.glowRed       = juce::Colour(0x30cf222e);

    // Knob colors (light)
    lightScheme.knobBody      = juce::Colour(0xffe6eaef);
    lightScheme.knobBodyLight = juce::Colour(0xfff6f8fa);
    lightScheme.knobArcBg     = juce::Colour(0xffd0d7de);
    lightScheme.knobIndicator = juce::Colour(0xff24292f);

    // Text
    lightScheme.textPrimary   = juce::Colour(0xff24292f);
    lightScheme.textSecondary = juce::Colour(0xff57606a);
    lightScheme.textMuted     = juce::Colour(0xff8c959f);
    lightScheme.textDisabled  = juce::Colour(0xffafb8c1);

    // Borders
    lightScheme.border        = juce::Colour(0xffd0d7de);
    lightScheme.borderLight   = juce::Colour(0xffe1e4e8);
    lightScheme.borderGlow    = juce::Colour(0x300969da);

    // Meters
    lightScheme.meterGreen    = juce::Colour(0xff1a7f37);
    lightScheme.meterYellow   = juce::Colour(0xffbf8700);
    lightScheme.meterRed      = juce::Colour(0xffcf222e);
    lightScheme.meterBg       = juce::Colour(0xffeaeef2);

    currentColors = &darkScheme;
}

ThemeManager& ThemeManager::getInstance()
{
    static ThemeManager instance;
    return instance;
}

void ThemeManager::setTheme(Theme theme)
{
    if (theme == currentTheme)
        return;

    currentTheme = theme;
    currentColors = (theme == Theme::Dark) ? &darkScheme : &lightScheme;

    listeners.call(&Listener::themeChanged);
}

void ThemeManager::addListener(Listener* listener)
{
    listeners.add(listener);
}

void ThemeManager::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

//==============================================================================
// ProgFlowLookAndFeel Implementation

ProgFlowLookAndFeel::ProgFlowLookAndFeel()
{
    ThemeManager::getInstance().addListener(this);
    updateColours();
}

ProgFlowLookAndFeel::~ProgFlowLookAndFeel()
{
    ThemeManager::getInstance().removeListener(this);
}

void ProgFlowLookAndFeel::themeChanged()
{
    updateColours();
}

void ProgFlowLookAndFeel::updateColours()
{
    setColour(juce::ResizableWindow::backgroundColourId, ProgFlowColours::bgPrimary());
    setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
    setColour(juce::TextButton::buttonOnColourId, ProgFlowColours::accentBlue());
    setColour(juce::TextButton::textColourOffId, ProgFlowColours::textPrimary());
    setColour(juce::TextButton::textColourOnId, ProgFlowColours::textPrimary());
    setColour(juce::Label::textColourId, ProgFlowColours::textPrimary());
    setColour(juce::Slider::thumbColourId, ProgFlowColours::accentBlue());
    setColour(juce::Slider::trackColourId, ProgFlowColours::bgTertiary());
    setColour(juce::ComboBox::backgroundColourId, ProgFlowColours::bgTertiary());
    setColour(juce::ComboBox::textColourId, ProgFlowColours::textPrimary());
    setColour(juce::ComboBox::outlineColourId, ProgFlowColours::border());
    setColour(juce::PopupMenu::backgroundColourId, ProgFlowColours::bgSecondary());
    setColour(juce::PopupMenu::textColourId, ProgFlowColours::textPrimary());
    setColour(juce::PopupMenu::highlightedBackgroundColourId, ProgFlowColours::accentBlue());
    setColour(juce::ScrollBar::thumbColourId, ProgFlowColours::bgHover());
    setColour(juce::ScrollBar::trackColourId, ProgFlowColours::bgTertiary());
    setColour(juce::TabbedComponent::backgroundColourId, ProgFlowColours::bgSecondary());
    setColour(juce::TabbedButtonBar::tabOutlineColourId, ProgFlowColours::border());
    setColour(juce::TabbedButtonBar::frontOutlineColourId, ProgFlowColours::accentBlue());
    setColour(juce::AlertWindow::backgroundColourId, ProgFlowColours::bgSecondary());
    setColour(juce::AlertWindow::textColourId, ProgFlowColours::textPrimary());
    setColour(juce::AlertWindow::outlineColourId, ProgFlowColours::border());
    setColour(juce::TextEditor::backgroundColourId, ProgFlowColours::bgTertiary());
    setColour(juce::TextEditor::textColourId, ProgFlowColours::textPrimary());
    setColour(juce::TextEditor::outlineColourId, ProgFlowColours::border());
    setColour(juce::TextEditor::focusedOutlineColourId, ProgFlowColours::accentBlue());
}

void ProgFlowLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                                juce::Button& button,
                                                const juce::Colour& backgroundColour,
                                                bool shouldDrawButtonAsHighlighted,
                                                bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    auto cornerSize = static_cast<float>(ProgFlowSpacing::BUTTON_CORNER_RADIUS);

    juce::Colour bgColour = backgroundColour;
    bool isToggled = button.getToggleState();

    if (shouldDrawButtonAsDown)
    {
        bgColour = bgColour.darker(0.2f);
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        bgColour = isToggled ? bgColour.brighter(0.1f) : ProgFlowColours::bgHover();
    }

    // Subtle glow for toggled buttons
    if (isToggled && !shouldDrawButtonAsDown)
    {
        g.setColour(ProgFlowColours::glowBlue());
        g.fillRoundedRectangle(bounds.expanded(2.0f), cornerSize + 2.0f);
    }

    // Gradient background for depth
    juce::ColourGradient gradient(
        bgColour.brighter(0.05f), bounds.getX(), bounds.getY(),
        bgColour.darker(0.05f), bounds.getX(), bounds.getBottom(),
        false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerSize);

    // Subtle top highlight
    g.setColour(juce::Colour(0x08ffffff));
    g.drawHorizontalLine(static_cast<int>(bounds.getY() + 1), bounds.getX() + cornerSize, bounds.getRight() - cornerSize);

    // Border
    g.setColour(isToggled ? ProgFlowColours::accentBlue().withAlpha(0.5f) : ProgFlowColours::glassBorder());
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
}

juce::Font ProgFlowLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font(juce::jmin(14.0f, static_cast<float>(buttonHeight) * 0.6f));
}

void ProgFlowLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                            int x, int y, int width, int height,
                                            float sliderPosProportional,
                                            float rotaryStartAngle,
                                            float rotaryEndAngle,
                                            juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    bool isEnabled = slider.isEnabled();
    auto accentColour = isEnabled ? ProgFlowColours::accentBlue() : ProgFlowColours::textDisabled();

    // Outer glow (bloom effect) when enabled
    if (isEnabled && sliderPosProportional > 0.01f)
    {
        g.setColour(ProgFlowColours::glowBlue());
        g.fillEllipse(rx - 2, ry - 2, rw + 4, rw + 4);
    }

    // Background circle with gradient
    juce::ColourGradient knobGradient(
        ProgFlowColours::knobBodyLight(), centreX, centreY - radius * 0.5f,
        ProgFlowColours::knobBody(), centreX, centreY + radius,
        false);
    g.setGradientFill(knobGradient);
    g.fillEllipse(rx, ry, rw, rw);

    // Subtle inner shadow
    juce::ColourGradient innerShadow(
        juce::Colour(0x00000000), centreX, centreY,
        juce::Colour(0x30000000), centreX, centreY + radius,
        true);
    g.setGradientFill(innerShadow);
    g.fillEllipse(rx + 2, ry + 2, rw - 4, rw - 4);

    // Border ring
    g.setColour(ProgFlowColours::glassBorder());
    g.drawEllipse(rx, ry, rw, rw, 1.0f);

    // Arc background (inactive portion)
    juce::Path arcBgPath;
    arcBgPath.addCentredArc(centreX, centreY,
                            radius * 0.78f, radius * 0.78f,
                            0.0f,
                            rotaryStartAngle, rotaryEndAngle,
                            true);
    g.setColour(ProgFlowColours::knobArcBg());
    g.strokePath(arcBgPath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Arc (value indicator with glow)
    if (sliderPosProportional > 0.01f)
    {
        juce::Path arcPath;
        arcPath.addCentredArc(centreX, centreY,
                              radius * 0.78f, radius * 0.78f,
                              0.0f,
                              rotaryStartAngle, angle,
                              true);

        // Glow layer
        if (isEnabled)
        {
            g.setColour(ProgFlowColours::glowBlue());
            g.strokePath(arcPath, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Main arc
        g.setColour(accentColour);
        g.strokePath(arcPath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Position indicator dot
    auto indicatorRadius = radius * 0.65f;
    auto indicatorX = centreX + std::sin(angle) * indicatorRadius;
    auto indicatorY = centreY - std::cos(angle) * indicatorRadius;

    g.setColour(ProgFlowColours::knobIndicator());
    g.fillEllipse(indicatorX - 3.0f, indicatorY - 3.0f, 6.0f, 6.0f);
}

void ProgFlowLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                            int x, int y, int width, int height,
                                            float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                            const juce::Slider::SliderStyle style,
                                            juce::Slider& slider)
{
    bool isVertical = style == juce::Slider::LinearVertical || style == juce::Slider::LinearBarVertical;

    // Track
    auto trackWidth = isVertical ? 6.0f : static_cast<float>(height);
    auto trackHeight = isVertical ? static_cast<float>(height) : 6.0f;

    juce::Rectangle<float> track;
    if (isVertical)
        track = juce::Rectangle<float>(static_cast<float>(x) + (static_cast<float>(width) - trackWidth) * 0.5f, static_cast<float>(y), trackWidth, trackHeight);
    else
        track = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y) + (static_cast<float>(height) - trackHeight) * 0.5f, static_cast<float>(width), trackHeight);

    g.setColour(ProgFlowColours::bgTertiary());
    g.fillRoundedRectangle(track, 3.0f);

    // Filled portion
    juce::Rectangle<float> filled;
    if (isVertical)
        filled = juce::Rectangle<float>(track.getX(), sliderPos, track.getWidth(), track.getBottom() - sliderPos);
    else
        filled = juce::Rectangle<float>(track.getX(), track.getY(), sliderPos - track.getX(), track.getHeight());

    g.setColour(slider.isEnabled() ? ProgFlowColours::accentBlue() : ProgFlowColours::textDisabled());
    g.fillRoundedRectangle(filled, 3.0f);

    // Thumb
    auto thumbSize = 14.0f;
    juce::Rectangle<float> thumb;
    if (isVertical)
        thumb = juce::Rectangle<float>(static_cast<float>(x) + (static_cast<float>(width) - thumbSize) * 0.5f, sliderPos - thumbSize * 0.5f, thumbSize, thumbSize);
    else
        thumb = juce::Rectangle<float>(sliderPos - thumbSize * 0.5f, static_cast<float>(y) + (static_cast<float>(height) - thumbSize) * 0.5f, thumbSize, thumbSize);

    g.setColour(ProgFlowColours::textPrimary());
    g.fillEllipse(thumb);
}

void ProgFlowLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));

    auto textArea = label.getLocalBounds();

    g.setColour(label.findColour(juce::Label::textColourId));
    g.setFont(label.getFont());
    g.drawText(label.getText(), textArea, label.getJustificationType(), true);
}

void ProgFlowLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
                                        bool /*isButtonDown*/, int /*buttonX*/, int /*buttonY*/,
                                        int /*buttonW*/, int /*buttonH*/,
                                        juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(1.0f);
    auto cornerSize = 4.0f;

    // Background
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, cornerSize);

    // Border
    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

    // Arrow
    auto arrowZone = juce::Rectangle<float>(static_cast<float>(width) - 20.0f, 0.0f, 15.0f, static_cast<float>(height));
    juce::Path arrow;
    arrow.addTriangle(arrowZone.getCentreX() - 4.0f, arrowZone.getCentreY() - 2.0f,
                      arrowZone.getCentreX() + 4.0f, arrowZone.getCentreY() - 2.0f,
                      arrowZone.getCentreX(), arrowZone.getCentreY() + 3.0f);

    g.setColour(box.findColour(juce::ComboBox::textColourId));
    g.fillPath(arrow);
}
