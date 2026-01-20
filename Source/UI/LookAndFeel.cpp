#include "LookAndFeel.h"

//==============================================================================
// ThemeManager Implementation

ThemeManager::ThemeManager()
{
    // Dark theme (default)
    darkScheme.bgPrimary     = juce::Colour(0xff1a1a1a);
    darkScheme.bgSecondary   = juce::Colour(0xff242424);
    darkScheme.bgTertiary    = juce::Colour(0xff2d2d2d);
    darkScheme.bgHover       = juce::Colour(0xff363636);
    darkScheme.sectionBg     = juce::Colour(0xff2a2a2a);  // Lighter raised sections
    darkScheme.surfaceBg     = juce::Colour(0xff222222);  // Raised surfaces
    darkScheme.dividerLine   = juce::Colour(0xff3a3a3a);  // Thin section dividers
    darkScheme.accentBlue    = juce::Colour(0xff3b82f6);
    darkScheme.accentGreen   = juce::Colour(0xff10b981);
    darkScheme.accentOrange  = juce::Colour(0xfff59e0b);
    darkScheme.accentRed     = juce::Colour(0xffef4444);
    darkScheme.textPrimary   = juce::Colour(0xffffffff);
    darkScheme.textSecondary = juce::Colour(0xffa0a0a0);
    darkScheme.textMuted     = juce::Colour(0xff666666);  // Section headers
    darkScheme.textDisabled  = juce::Colour(0xff606060);
    darkScheme.border        = juce::Colour(0xff404040);
    darkScheme.borderLight   = juce::Colour(0xff505050);
    darkScheme.meterGreen    = juce::Colour(0xff22c55e);
    darkScheme.meterYellow   = juce::Colour(0xffeab308);
    darkScheme.meterRed      = juce::Colour(0xffef4444);
    darkScheme.meterBg       = juce::Colour(0xff0a0a0a);  // Very dark meter background

    // Light theme
    lightScheme.bgPrimary     = juce::Colour(0xfff5f5f5);
    lightScheme.bgSecondary   = juce::Colour(0xffffffff);
    lightScheme.bgTertiary    = juce::Colour(0xffe8e8e8);
    lightScheme.bgHover       = juce::Colour(0xffd0d0d0);
    lightScheme.sectionBg     = juce::Colour(0xffebebeb);  // Slightly darker sections
    lightScheme.surfaceBg     = juce::Colour(0xfffafafa);  // Raised surfaces
    lightScheme.dividerLine   = juce::Colour(0xffd0d0d0);  // Thin section dividers
    lightScheme.accentBlue    = juce::Colour(0xff2563eb);
    lightScheme.accentGreen   = juce::Colour(0xff059669);
    lightScheme.accentOrange  = juce::Colour(0xffd97706);
    lightScheme.accentRed     = juce::Colour(0xffdc2626);
    lightScheme.textPrimary   = juce::Colour(0xff1a1a1a);
    lightScheme.textSecondary = juce::Colour(0xff606060);
    lightScheme.textMuted     = juce::Colour(0xff909090);  // Section headers
    lightScheme.textDisabled  = juce::Colour(0xffa0a0a0);
    lightScheme.border        = juce::Colour(0xffc0c0c0);
    lightScheme.borderLight   = juce::Colour(0xffd0d0d0);
    lightScheme.meterGreen    = juce::Colour(0xff16a34a);
    lightScheme.meterYellow   = juce::Colour(0xffca8a04);
    lightScheme.meterRed      = juce::Colour(0xffdc2626);
    lightScheme.meterBg       = juce::Colour(0xffd0d0d0);  // Light meter background

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

    if (shouldDrawButtonAsDown)
        bgColour = bgColour.darker(0.15f);
    else if (shouldDrawButtonAsHighlighted)
        bgColour = ProgFlowColours::bgHover();

    // Draw background only - no border for cleaner look
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, cornerSize);
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
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Background circle
    g.setColour(ProgFlowColours::bgTertiary());
    g.fillEllipse(rx, ry, rw, rw);

    // Border
    g.setColour(ProgFlowColours::border());
    g.drawEllipse(rx, ry, rw, rw, 1.0f);

    // Arc (value indicator)
    juce::Path arcPath;
    arcPath.addCentredArc(centreX, centreY,
                          radius * 0.85f, radius * 0.85f,
                          0.0f,
                          rotaryStartAngle, angle,
                          true);

    g.setColour(slider.isEnabled() ? ProgFlowColours::accentBlue() : ProgFlowColours::textDisabled());
    g.strokePath(arcPath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pointer
    juce::Path pointerPath;
    auto pointerLength = radius * 0.6f;
    auto pointerThickness = 3.0f;

    pointerPath.addRectangle(-pointerThickness * 0.5f, -radius * 0.85f, pointerThickness, pointerLength);
    pointerPath.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

    g.setColour(ProgFlowColours::textPrimary());
    g.fillPath(pointerPath);
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
