#include "ToastManager.h"
#include "LookAndFeel.h"

ToastManager::ToastManager()
{
    setInterceptsMouseClicks(false, false);
    setAlwaysOnTop(true);
}

ToastManager::~ToastManager()
{
    stopTimer();
}

ToastManager& ToastManager::getInstance()
{
    static ToastManager instance;
    return instance;
}

void ToastManager::showToast(const juce::String& message, ToastType type, int durationMs)
{
    Toast toast;
    toast.message = message;
    toast.type = type;
    toast.showTime = juce::Time::currentTimeMillis();
    toast.duration = durationMs;
    toast.alpha = 1.0f;

    toasts.push_back(toast);

    // Start timer if not running
    if (!isTimerRunning())
        startTimerHz(30);

    repaint();

    DBG("Toast shown: " << message);
}

void ToastManager::clearAll()
{
    toasts.clear();
    stopTimer();
    repaint();
}

void ToastManager::paint(juce::Graphics& g)
{
    // Draw toasts from bottom to top
    int y = getHeight() - TOAST_MARGIN;

    for (auto it = toasts.rbegin(); it != toasts.rend(); ++it)
    {
        auto& toast = *it;

        // Calculate toast bounds
        int width = std::min(400, getWidth() - 2 * TOAST_MARGIN);
        int x = (getWidth() - width) / 2;
        y -= TOAST_HEIGHT;

        juce::Rectangle<int> bounds(x, y, width, TOAST_HEIGHT);

        // Draw rounded background
        auto colour = getToastColour(toast.type).withAlpha(toast.alpha * 0.95f);
        g.setColour(colour);
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);

        // Draw border
        g.setColour(colour.brighter(0.2f).withAlpha(toast.alpha));
        g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 1.5f);

        // Draw icon based on type
        juce::String icon;
        switch (toast.type)
        {
            case ToastType::Success: icon = juce::CharPointer_UTF8("\xe2\x9c\x93"); break;  // checkmark
            case ToastType::Warning: icon = juce::CharPointer_UTF8("\xe2\x9a\xa0"); break;  // warning
            case ToastType::Error:   icon = juce::CharPointer_UTF8("\xe2\x9c\x97"); break;  // X
            default:                 icon = juce::CharPointer_UTF8("\xe2\x84\xb9"); break;  // info
        }

        g.setColour(juce::Colours::white.withAlpha(toast.alpha));
        g.setFont(16.0f);
        g.drawText(icon, bounds.removeFromLeft(35).reduced(8, 0), juce::Justification::centred, false);

        // Draw message
        g.setFont(13.0f);
        g.drawText(toast.message, bounds.reduced(5, 0), juce::Justification::centredLeft, true);

        y -= TOAST_MARGIN;
    }
}

void ToastManager::resized()
{
    // Component fills parent - toasts are positioned in paint()
}

void ToastManager::timerCallback()
{
    juce::int64 now = juce::Time::currentTimeMillis();
    bool needsRepaint = false;
    bool anyActive = false;

    // Update toast states
    for (auto& toast : toasts)
    {
        juce::int64 elapsed = now - toast.showTime;
        juce::int64 fadeStart = toast.duration - FADE_DURATION_MS;

        if (elapsed >= fadeStart)
        {
            // Fading out
            float fadeProgress = static_cast<float>(elapsed - fadeStart) / static_cast<float>(FADE_DURATION_MS);
            toast.alpha = std::max(0.0f, 1.0f - fadeProgress);
            needsRepaint = true;
        }

        if (elapsed < toast.duration)
            anyActive = true;
    }

    // Remove expired toasts
    toasts.erase(
        std::remove_if(toasts.begin(), toasts.end(),
            [now](const Toast& t) { return (now - t.showTime) >= t.duration; }),
        toasts.end()
    );

    if (!anyActive && toasts.empty())
    {
        stopTimer();
    }

    if (needsRepaint)
        repaint();
}

juce::Colour ToastManager::getToastColour(ToastType type) const
{
    switch (type)
    {
        case ToastType::Success: return ProgFlowColours::accentGreen();
        case ToastType::Warning: return juce::Colour(0xfff59e0b);  // Amber
        case ToastType::Error:   return ProgFlowColours::accentRed();
        default:                 return ProgFlowColours::accentBlue();  // Info
    }
}
