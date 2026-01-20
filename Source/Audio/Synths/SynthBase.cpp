#include "SynthBase.h"

SynthBase::SynthBase()
{
}

void SynthBase::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    sampleRate = newSampleRate;
    samplesPerBlock = newSamplesPerBlock;
}

void SynthBase::releaseResources()
{
    killAllNotes();
}

void SynthBase::allNotesOff()
{
    // Copy active notes to avoid modifying set while iterating
    auto notesToRelease = activeNotes;
    for (int note : notesToRelease)
    {
        noteOff(note, 0);
    }
}

void SynthBase::killAllNotes()
{
    activeNotes.clear();
}

//==============================================================================
// MIDI conversion utilities

float SynthBase::midiToFrequency(int midiNote)
{
    // A4 (MIDI 69) = 440 Hz
    return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
}

int SynthBase::frequencyToMidi(float frequency)
{
    if (frequency <= 0.0f) return 0;
    return static_cast<int>(std::round(69.0f + 12.0f * std::log2(frequency / 440.0f)));
}

juce::String SynthBase::midiToNoteName(int midiNote)
{
    static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int octave = (midiNote / 12) - 1;
    int noteIndex = midiNote % 12;
    return juce::String(noteNames[noteIndex]) + juce::String(octave);
}

//==============================================================================
// Parameter management

void SynthBase::addParameter(const juce::String& id, const juce::String& name,
                            float defaultValue, float minValue, float maxValue,
                            float step)
{
    SynthParameter param;
    param.name = name;
    param.value = defaultValue;
    param.defaultValue = defaultValue;
    param.minValue = minValue;
    param.maxValue = maxValue;
    param.step = step;

    juce::ScopedLock lock(parameterLock);
    parameters[id] = param;
}

void SynthBase::addEnumParameter(const juce::String& id, const juce::String& name,
                                const juce::StringArray& options, int defaultIndex)
{
    SynthParameter param;
    param.name = name;
    param.options = options;
    param.enumIndex = defaultIndex;
    param.value = static_cast<float>(defaultIndex);
    param.defaultValue = static_cast<float>(defaultIndex);
    param.minValue = 0.0f;
    param.maxValue = static_cast<float>(options.size() - 1);

    juce::ScopedLock lock(parameterLock);
    parameters[id] = param;
}

void SynthBase::setParameter(const juce::String& name, float value)
{
    {
        juce::ScopedLock lock(parameterLock);
        auto it = parameters.find(name);
        if (it == parameters.end()) return;

        auto& param = it->second;
        value = juce::jlimit(param.minValue, param.maxValue, value);
        if (param.step > 0.0f)
            value = std::round(value / param.step) * param.step;
        param.value = value;
    }

    onParameterChanged(name, value);
}

void SynthBase::setParameterEnum(const juce::String& name, int index)
{
    {
        juce::ScopedLock lock(parameterLock);
        auto it = parameters.find(name);
        if (it == parameters.end()) return;

        auto& param = it->second;
        if (!param.isEnum()) return;

        index = juce::jlimit(0, param.options.size() - 1, index);
        param.enumIndex = index;
        param.value = static_cast<float>(index);
    }

    onParameterEnumChanged(name, index);
}

void SynthBase::setParameterEnum(const juce::String& name, const juce::String& optionName)
{
    int foundIndex = -1;

    {
        juce::ScopedLock lock(parameterLock);
        auto it = parameters.find(name);
        if (it == parameters.end()) return;

        auto& param = it->second;
        if (!param.isEnum()) return;

        int index = param.options.indexOf(optionName);
        if (index >= 0)
        {
            param.enumIndex = index;
            param.value = static_cast<float>(index);
            foundIndex = index;
        }
    }

    // Callback after lock released
    if (foundIndex >= 0)
        onParameterEnumChanged(name, foundIndex);
}

float SynthBase::getParameter(const juce::String& name) const
{
    juce::ScopedLock lock(parameterLock);
    auto it = parameters.find(name);
    if (it == parameters.end()) return 0.0f;
    return it->second.value;
}

int SynthBase::getParameterEnum(const juce::String& name) const
{
    juce::ScopedLock lock(parameterLock);
    auto it = parameters.find(name);
    if (it == parameters.end()) return 0;
    return it->second.enumIndex;
}

const SynthParameter* SynthBase::getParameterInfo(const juce::String& name) const
{
    juce::ScopedLock lock(parameterLock);
    auto it = parameters.find(name);
    if (it == parameters.end()) return nullptr;
    return &it->second;
}

std::vector<juce::String> SynthBase::getParameterNames() const
{
    juce::ScopedLock lock(parameterLock);
    std::vector<juce::String> names;
    names.reserve(parameters.size());
    for (const auto& pair : parameters)
        names.push_back(pair.first);
    return names;
}

//==============================================================================
// Presets

void SynthBase::loadPreset(const SynthPreset& preset)
{
    for (const auto& pair : preset.values)
    {
        setParameter(pair.first, pair.second);
    }
    for (const auto& pair : preset.enumValues)
    {
        setParameterEnum(pair.first, pair.second);
    }
}

void SynthBase::loadPreset(int index)
{
    auto presets = getPresets();
    if (index >= 0 && index < static_cast<int>(presets.size()))
    {
        loadPreset(presets[index]);
        currentPresetIndex = index;
    }
}

void SynthBase::loadPreset(const juce::String& presetName)
{
    auto presets = getPresets();
    for (int i = 0; i < static_cast<int>(presets.size()); ++i)
    {
        if (presets[i].name.equalsIgnoreCase(presetName))
        {
            loadPreset(presets[i]);
            currentPresetIndex = i;
            return;
        }
    }
    // Preset not found - keep current state
}

juce::String SynthBase::getCurrentPresetName() const
{
    auto presets = getPresets();
    if (currentPresetIndex >= 0 && currentPresetIndex < static_cast<int>(presets.size()))
    {
        return presets[currentPresetIndex].name;
    }
    return "Custom";
}

std::map<juce::String, float> SynthBase::getParameters() const
{
    std::map<juce::String, float> result;
    juce::ScopedLock lock(parameterLock);
    for (const auto& pair : parameters)
    {
        result[pair.first] = pair.second.value;
    }
    return result;
}

SynthPreset SynthBase::getCurrentAsPreset(const juce::String& name) const
{
    SynthPreset preset;
    preset.name = name;

    juce::ScopedLock lock(parameterLock);
    for (const auto& pair : parameters)
    {
        if (pair.second.isEnum())
            preset.enumValues[pair.first] = pair.second.enumIndex;
        else
            preset.values[pair.first] = pair.second.value;
    }

    return preset;
}

//==============================================================================
// MIDI processing

void SynthBase::processMidiMessages(juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        const int samplePosition = metadata.samplePosition;

        if (msg.isNoteOn())
        {
            noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), samplePosition);
        }
        else if (msg.isNoteOff())
        {
            noteOff(msg.getNoteNumber(), samplePosition);
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            allNotesOff();
        }
        else if (msg.isController())
        {
            // Handle CC messages - mod wheel, etc.
            int ccNum = msg.getControllerNumber();
            int ccVal = msg.getControllerValue();

            // Mod wheel (CC1) - could map to filter or LFO depth
            if (ccNum == 1)
            {
                // Subclasses can override to handle this
            }
        }
    }
}
