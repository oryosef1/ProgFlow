#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Audio/AudioClip.h"
#include "../../Audio/AudioFileLoader.h"

/**
 * WaveformComponent - Displays audio waveform visualization
 *
 * Features:
 * - Renders waveform from AudioClip data
 * - Supports zoom levels
 * - Shows clip name overlay
 * - Handles resizing efficiently
 * - Caches waveform for performance
 */
class WaveformComponent : public juce::Component,
                          private juce::ChangeListener
{
public:
    WaveformComponent();
    ~WaveformComponent() override;

    //==========================================================================
    // Audio source

    /**
     * Set the audio clip to display
     */
    void setAudioClip(AudioClip* clip);

    /**
     * Get the current audio clip
     */
    AudioClip* getAudioClip() const { return audioClip; }

    //==========================================================================
    // Display settings

    /**
     * Set the waveform color
     */
    void setWaveformColour(juce::Colour colour);

    /**
     * Set the background color
     */
    void setBackgroundColour(juce::Colour colour);

    /**
     * Show/hide clip name overlay
     */
    void setShowName(bool show);

    /**
     * Set pixels per second (for zoom)
     */
    void setPixelsPerSecond(double pps);

    /**
     * Get pixels per second
     */
    double getPixelsPerSecond() const { return pixelsPerSecond; }

    //==========================================================================
    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Audio clip reference
    AudioClip* audioClip = nullptr;

    // Thumbnail for efficient waveform rendering
    juce::AudioThumbnailCache thumbnailCache{5};
    std::unique_ptr<juce::AudioThumbnail> thumbnail;

    // Display colors
    juce::Colour waveformColour{0xff3b82f6};  // Blue
    juce::Colour backgroundColour{juce::Colours::transparentBlack};

    // Display options
    bool showName = true;
    double pixelsPerSecond = 100.0;

    // ChangeListener for thumbnail updates
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // Update thumbnail from clip data
    void updateThumbnail();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent)
};
