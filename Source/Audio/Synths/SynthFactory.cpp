#include "SynthFactory.h"
#include "AnalogSynth.h"
#include "FMSynth.h"
#include "ProSynth/ProSynth.h"
#include "Sampler.h"
#include "SoundFontPlayer.h"
#include "DrumSynth.h"

std::unique_ptr<SynthBase> SynthFactory::createSynth(SynthType type)
{
    switch (type)
    {
        case SynthType::Analog:
            return std::make_unique<AnalogSynth>();

        case SynthType::FM:
            return std::make_unique<FMSynth>();

        case SynthType::Pro:
            return std::make_unique<ProSynth>();

        case SynthType::Sampler:
            return std::make_unique<Sampler>();

        case SynthType::SoundFont:
            return std::make_unique<SoundFontPlayer>();

        case SynthType::Drums:
            return std::make_unique<DrumSynth>();

        default:
            return std::make_unique<AnalogSynth>();
    }
}

juce::String SynthFactory::getSynthName(SynthType type)
{
    switch (type)
    {
        case SynthType::Analog:    return "Analog";
        case SynthType::FM:        return "FM";
        case SynthType::Pro:       return "Pro";
        case SynthType::Sampler:   return "Sampler";
        case SynthType::SoundFont: return "SoundFont";
        case SynthType::Drums:     return "Drums";
        default:                   return "Unknown";
    }
}

SynthType SynthFactory::getSynthType(int index)
{
    if (index < 0 || index >= static_cast<int>(SynthType::COUNT))
        return SynthType::Analog;

    return static_cast<SynthType>(index);
}

juce::StringArray SynthFactory::getAllSynthNames()
{
    juce::StringArray names;
    for (int i = 0; i < static_cast<int>(SynthType::COUNT); ++i)
    {
        names.add(getSynthName(static_cast<SynthType>(i)));
    }
    return names;
}

SynthType SynthFactory::getSynthType(const juce::String& name)
{
    juce::String lower = name.toLowerCase();

    if (lower == "analog")   return SynthType::Analog;
    if (lower == "fm")       return SynthType::FM;
    if (lower == "pro" || lower == "prosynth") return SynthType::Pro;
    if (lower == "sampler")  return SynthType::Sampler;
    if (lower == "soundfont" || lower == "sf2") return SynthType::SoundFont;
    if (lower == "drums" || lower == "drum" || lower == "drumsynth") return SynthType::Drums;

    // Legacy synth names -> map to Pro (the versatile replacement)
    if (lower == "poly pad" || lower == "polypad") return SynthType::Pro;
    if (lower == "organ")    return SynthType::Pro;
    if (lower == "string")   return SynthType::Pro;

    return SynthType::Analog; // Default
}
