#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>

/**
 * ToastManager - Manages temporary notification messages
 *
 * Shows toast messages that auto-dismiss after a timeout.
 * Supports multiple toast types: info, success, warning, error.
 */
class ToastManager : public juce::Component,
                      private juce::Timer
{
public:
    enum class ToastType { Info, Success, Warning, Error };

    ToastManager();
    ~ToastManager() override;

    // Show a toast message (duration in ms, default 3000)
    void showToast(const juce::String& message, ToastType type = ToastType::Info, int durationMs = 3000);

    // Clear all toasts
    void clearAll();

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Singleton access
    static ToastManager& getInstance();

private:
    struct Toast
    {
        juce::String message;
        ToastType type;
        juce::int64 showTime;
        int duration;
        float alpha = 1.0f;
    };

    std::vector<Toast> toasts;

    static constexpr int TOAST_HEIGHT = 40;
    static constexpr int TOAST_MARGIN = 10;
    static constexpr int FADE_DURATION_MS = 200;

    void timerCallback() override;
    juce::Colour getToastColour(ToastType type) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToastManager)
};
