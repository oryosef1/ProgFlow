#include "WaveformComponent.h"

WaveformComponent::WaveformComponent()
{
    // Create thumbnail with format manager from global loader
    thumbnail = std::make_unique<juce::AudioThumbnail>(
        512,  // Samples per thumbnail point
        getAudioFileLoader().getFormatManager(),
        thumbnailCache
    );

    thumbnail->addChangeListener(this);
}

WaveformComponent::~WaveformComponent()
{
    if (thumbnail)
        thumbnail->removeChangeListener(this);
}

void WaveformComponent::setAudioClip(AudioClip* clip)
{
    if (audioClip == clip)
        return;

    audioClip = clip;
    updateThumbnail();
    repaint();
}

void WaveformComponent::setWaveformColour(juce::Colour colour)
{
    waveformColour = colour;
    repaint();
}

void WaveformComponent::setBackgroundColour(juce::Colour colour)
{
    backgroundColour = colour;
    repaint();
}

void WaveformComponent::setShowName(bool show)
{
    showName = show;
    repaint();
}

void WaveformComponent::setPixelsPerSecond(double pps)
{
    if (std::abs(pixelsPerSecond - pps) < 0.01)
        return;

    pixelsPerSecond = pps;
    repaint();
}

void WaveformComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Draw background
    if (!backgroundColour.isTransparent())
    {
        g.setColour(backgroundColour);
        g.fillRect(bounds);
    }

    // Draw waveform
    if (thumbnail && thumbnail->getTotalLength() > 0.0)
    {
        g.setColour(waveformColour);

        // Calculate visible time range
        double durationSeconds = thumbnail->getTotalLength();

        // Draw the waveform thumbnail
        thumbnail->drawChannels(g,
                                bounds,
                                0.0,        // Start time
                                durationSeconds,  // End time
                                1.0f);      // Vertical zoom
    }
    else if (audioClip && audioClip->hasAudio())
    {
        // Fallback: draw simple waveform from buffer
        g.setColour(waveformColour);

        const auto& buffer = audioClip->getAudioBuffer();
        int numSamples = buffer.getNumSamples();
        int numChannels = buffer.getNumChannels();

        if (numSamples > 0 && numChannels > 0)
        {
            float centerY = bounds.getCentreY();
            float halfHeight = bounds.getHeight() * 0.4f;

            // Calculate samples per pixel
            int samplesPerPixel = std::max(1, numSamples / bounds.getWidth());

            juce::Path waveformPath;
            bool pathStarted = false;

            for (int x = 0; x < bounds.getWidth(); ++x)
            {
                int startSample = x * samplesPerPixel;
                int endSample = std::min(startSample + samplesPerPixel, numSamples);

                // Find min/max in this pixel range
                float minVal = 0.0f, maxVal = 0.0f;

                for (int s = startSample; s < endSample; ++s)
                {
                    for (int ch = 0; ch < numChannels; ++ch)
                    {
                        float sample = buffer.getSample(ch, s);
                        minVal = std::min(minVal, sample);
                        maxVal = std::max(maxVal, sample);
                    }
                }

                float y1 = centerY - maxVal * halfHeight;
                float y2 = centerY - minVal * halfHeight;

                if (!pathStarted)
                {
                    waveformPath.startNewSubPath(static_cast<float>(x), y1);
                    pathStarted = true;
                }

                waveformPath.lineTo(static_cast<float>(x), y1);
                waveformPath.lineTo(static_cast<float>(x), y2);
            }

            g.strokePath(waveformPath, juce::PathStrokeType(1.0f));
        }
    }

    // Draw clip name
    if (showName && audioClip)
    {
        auto name = audioClip->getName();
        if (name.isNotEmpty())
        {
            g.setColour(juce::Colours::white);
            g.setFont(12.0f);
            g.drawText(name, bounds.reduced(4), juce::Justification::topLeft, true);
        }
    }
}

void WaveformComponent::resized()
{
    // No child components to layout
}

void WaveformComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == thumbnail.get())
    {
        repaint();
    }
}

void WaveformComponent::updateThumbnail()
{
    if (!thumbnail)
        return;

    thumbnail->clear();

    if (audioClip && audioClip->hasAudio())
    {
        const auto& buffer = audioClip->getAudioBuffer();
        double sampleRate = audioClip->getSampleRate();

        if (buffer.getNumSamples() > 0 && sampleRate > 0)
        {
            // Set the thumbnail from the audio buffer
            thumbnail->reset(buffer.getNumChannels(), sampleRate, buffer.getNumSamples());
            thumbnail->addBlock(0, buffer, 0, buffer.getNumSamples());
        }
    }
}
