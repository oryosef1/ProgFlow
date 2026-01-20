#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../Project/ProjectSerializer.h"

ProgFlowPluginProcessor::ProgFlowPluginProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    DBG("ProgFlowPluginProcessor created");
}

ProgFlowPluginProcessor::~ProgFlowPluginProcessor()
{
    DBG("ProgFlowPluginProcessor destroyed");
}

void ProgFlowPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    audioEngine.prepareToPlay(samplesPerBlock, sampleRate);
}

void ProgFlowPluginProcessor::releaseResources()
{
    audioEngine.releaseResources();
}

void ProgFlowPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Sync to host transport
    syncToHost();

    // Clear any input (we generate our own audio)
    buffer.clear();

    // Process MIDI input to the engine
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            audioEngine.synthNoteOn(msg.getNoteNumber(), msg.getFloatVelocity());
        }
        else if (msg.isNoteOff())
        {
            audioEngine.synthNoteOff(msg.getNoteNumber());
        }
    }
    midiMessages.clear();

    // Process audio through the engine
    juce::AudioSourceChannelInfo info(&buffer, 0, buffer.getNumSamples());
    audioEngine.getNextAudioBlock(info);
}

void ProgFlowPluginProcessor::syncToHost()
{
    if (auto* playHead = getPlayHead())
    {
        if (auto posInfo = playHead->getPosition())
        {
            // Sync BPM
            if (auto bpm = posInfo->getBpm())
            {
                if (std::abs(*bpm - lastHostBpm) > 0.1)
                {
                    audioEngine.setBpm(*bpm);
                    lastHostBpm = *bpm;
                }
            }

            // Sync transport state
            bool hostPlaying = posInfo->getIsPlaying();
            if (hostPlaying != wasPlaying)
            {
                if (hostPlaying)
                {
                    // Sync position before playing
                    if (auto ppqPos = posInfo->getPpqPosition())
                    {
                        audioEngine.setPositionInBeats(*ppqPos);
                    }
                    audioEngine.play();
                }
                else
                {
                    audioEngine.stop();
                }
                wasPlaying = hostPlaying;
            }
            else if (hostPlaying)
            {
                // While playing, keep position synced
                if (auto ppqPos = posInfo->getPpqPosition())
                {
                    // Only sync if significantly out of sync (> 1 beat)
                    double enginePos = audioEngine.getPositionInBeats();
                    if (std::abs(*ppqPos - enginePos) > 1.0)
                    {
                        audioEngine.setPositionInBeats(*ppqPos);
                    }
                }
            }
        }
    }
}

juce::AudioProcessorEditor* ProgFlowPluginProcessor::createEditor()
{
    return new ProgFlowPluginEditor(*this);
}

void ProgFlowPluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Serialize the audio engine state using ProjectSerializer
    juce::String json = ProjectSerializer::serializeFromEngine(
        audioEngine, "Plugin State", audioEngine.getBpm(), 4, 4);

    juce::MemoryOutputStream stream(destData, false);
    stream.writeString("ProgFlow_v2");  // Version marker
    stream.writeString(json);

    DBG("ProgFlow plugin state saved (" << json.length() << " bytes)");
}

void ProgFlowPluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
    auto header = stream.readString();

    if (header == "ProgFlow_v2")
    {
        // New format with full state
        juce::String json = stream.readString();
        juce::String projectName;
        double bpm;

        if (ProjectSerializer::deserializeToEngine(json, audioEngine, projectName, bpm))
        {
            audioEngine.setBpm(bpm);
            DBG("ProgFlow plugin state loaded successfully");
        }
        else
        {
            DBG("ProgFlow plugin state failed to parse");
        }
    }
    else if (header == "ProgFlow_v1")
    {
        // Legacy placeholder format - nothing to restore
        DBG("ProgFlow legacy state marker found (no state to restore)");
    }
}

//==============================================================================
// Plugin instantiation
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProgFlowPluginProcessor();
}
