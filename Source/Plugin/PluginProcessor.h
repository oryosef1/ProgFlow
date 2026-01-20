#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../Audio/AudioEngine.h"

/**
 * ProgFlowPluginProcessor - Plugin wrapper for ProgFlow DAW
 *
 * Allows ProgFlow to be loaded as a VST3/AU plugin inside another DAW.
 * Syncs to host tempo and transport, processes audio through the engine.
 */
class ProgFlowPluginProcessor : public juce::AudioProcessor
{
public:
    ProgFlowPluginProcessor();
    ~ProgFlowPluginProcessor() override;

    //==========================================================================
    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    //==========================================================================
    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    // Plugin info
    const juce::String getName() const override { return "ProgFlow"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==========================================================================
    // Programs (presets)
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override { juce::ignoreUnused(index); }
    const juce::String getProgramName(int index) override { juce::ignoreUnused(index); return {}; }
    void changeProgramName(int index, const juce::String& newName) override { juce::ignoreUnused(index, newName); }

    //==========================================================================
    // State save/restore
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==========================================================================
    // Access to engine
    AudioEngine& getAudioEngine() { return audioEngine; }

private:
    AudioEngine audioEngine;

    // Host sync state
    bool wasPlaying = false;
    double lastHostBpm = 120.0;

    // Sync transport to host
    void syncToHost();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProgFlowPluginProcessor)
};
