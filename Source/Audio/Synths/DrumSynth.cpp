#include "DrumSynth.h"
#include <cmath>

DrumSynth::DrumSynth()
{
    // Register master parameters
    addParameter("volume", "Volume", 0.8f, 0.0f, 1.0f);
    addParameter("swing", "Swing", 0.0f, 0.0f, 1.0f);

    // Add kit as an enum parameter for preset support
    addEnumParameter("kit", "Kit", getAvailableKits(), 0);

    // Initialize pads with default 808 kit
    initializePads();
    configure808Kit();
}

void DrumSynth::initializePads()
{
    // Standard 16-pad drum layout with GM note mapping
    struct PadConfig
    {
        const char* name;
        int midiNote;
        int chokeGroup;
        DrumPad::SoundType type;
    };

    static const PadConfig defaultLayout[NUM_PADS] = {
        {"Kick",      KICK_NOTE,      -1, DrumPad::SoundType::Kick},
        {"Snare",     SNARE_NOTE,     -1, DrumPad::SoundType::Snare},
        {"Closed HH", CLOSED_HH_NOTE,  1, DrumPad::SoundType::ClosedHiHat},
        {"Open HH",   OPEN_HH_NOTE,    1, DrumPad::SoundType::OpenHiHat},
        {"Tom Low",   TOM_LOW_NOTE,   -1, DrumPad::SoundType::Tom},
        {"Tom Mid",   TOM_MID_NOTE,   -1, DrumPad::SoundType::Tom},
        {"Tom High",  TOM_HIGH_NOTE,  -1, DrumPad::SoundType::Tom},
        {"Clap",      CLAP_NOTE,      -1, DrumPad::SoundType::Clap},
        {"Rim",       RIM_NOTE,       -1, DrumPad::SoundType::Rim},
        {"Crash",     CRASH_NOTE,      2, DrumPad::SoundType::Cymbal},
        {"Ride",      RIDE_NOTE,       2, DrumPad::SoundType::Cymbal},
        {"Cowbell",   COWBELL_NOTE,   -1, DrumPad::SoundType::Cowbell},
        {"Clave",     CLAVE_NOTE,     -1, DrumPad::SoundType::Clave},
        {"Shaker",    SHAKER_NOTE,    -1, DrumPad::SoundType::Shaker},
        {"Perc 1",    52,             -1, DrumPad::SoundType::Clave},
        {"Perc 2",    53,             -1, DrumPad::SoundType::Conga}
    };

    for (int i = 0; i < NUM_PADS; ++i)
    {
        pads[i].name = defaultLayout[i].name;
        pads[i].midiNote = defaultLayout[i].midiNote;
        pads[i].chokeGroup = defaultLayout[i].chokeGroup;
        pads[i].soundType = defaultLayout[i].type;
        pads[i].pitch = 1.0f;
        pads[i].decay = 0.5f;
        pads[i].tone = 0.5f;
        pads[i].level = 0.8f;
        pads[i].pan = 0.0f;
        pads[i].playing = false;
        pads[i].phase = 0.0f;
        pads[i].envelope = 0.0f;
    }
}

//==============================================================================
void DrumSynth::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    SynthBase::prepareToPlay(newSampleRate, newSamplesPerBlock);
    sampleRateLocal = newSampleRate;
}

void DrumSynth::processBlock(juce::AudioBuffer<float>& buffer,
                              juce::MidiBuffer& midiMessages)
{
    // Process MIDI
    processMidiMessages(midiMessages);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    const float masterVolume = getParameter("volume");

    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float leftSum = 0.0f;
        float rightSum = 0.0f;

        // Process each pad
        for (int p = 0; p < NUM_PADS; ++p)
        {
            auto& pad = pads[p];

            if (!pad.playing && pad.envelope <= 0.0001f)
                continue;

            // Generate sound based on type
            float output = 0.0f;
            float velocity = pad.velocity;  // Use stored velocity from noteOn

            switch (pad.soundType)
            {
                case DrumPad::SoundType::Kick:
                    output = synthesizeKick(pad, velocity);
                    break;
                case DrumPad::SoundType::Snare:
                    output = synthesizeSnare(pad, velocity);
                    break;
                case DrumPad::SoundType::ClosedHiHat:
                    output = synthesizeHiHat(pad, velocity, false);
                    break;
                case DrumPad::SoundType::OpenHiHat:
                    output = synthesizeHiHat(pad, velocity, true);
                    break;
                case DrumPad::SoundType::Tom:
                    output = synthesizeTom(pad, velocity);
                    break;
                case DrumPad::SoundType::Clap:
                    output = synthesizeClap(pad, velocity);
                    break;
                case DrumPad::SoundType::Rim:
                    output = synthesizeRim(pad, velocity);
                    break;
                case DrumPad::SoundType::Cymbal:
                    output = synthesizeCymbal(pad, velocity);
                    break;
                case DrumPad::SoundType::Cowbell:
                    output = synthesizeCowbell(pad, velocity);
                    break;
                case DrumPad::SoundType::Clave:
                    output = synthesizeClave(pad, velocity);
                    break;
                case DrumPad::SoundType::Shaker:
                    output = synthesizeShaker(pad, velocity);
                    break;
                case DrumPad::SoundType::Conga:
                    output = synthesizeConga(pad, velocity);
                    break;
            }

            // Apply level and pan
            output *= pad.level;

            float leftGain = pad.pan < 0 ? 1.0f : 1.0f - pad.pan;
            float rightGain = pad.pan > 0 ? 1.0f : 1.0f + pad.pan;

            leftSum += output * leftGain;
            rightSum += output * rightGain;

            // Check if pad has finished
            if (pad.envelope <= 0.0001f)
            {
                pad.playing = false;
            }
        }

        // Write to output
        leftChannel[sample] = leftSum * masterVolume;
        if (rightChannel)
            rightChannel[sample] = rightSum * masterVolume;
    }
}

void DrumSynth::releaseResources()
{
    SynthBase::releaseResources();
}

//==============================================================================
void DrumSynth::noteOn(int midiNote, float velocity, int /*sampleOffset*/)
{
    activeNotes.insert(midiNote);

    int padIndex = findPadByNote(midiNote);
    if (padIndex < 0)
        return;

    auto& pad = pads[padIndex];

    // Handle choke groups
    if (pad.chokeGroup >= 0)
    {
        chokePadsInGroup(pad.chokeGroup, padIndex);
    }

    // Trigger the pad
    pad.playing = true;
    pad.phase = 0.0f;
    pad.envelope = 1.0f;          // Envelope starts at full, decays to 0
    pad.velocity = velocity;       // Store velocity for amplitude scaling
    pad.noisePhase = 0.0f;
    pad.clickEnv = 1.0f;
}

void DrumSynth::noteOff(int midiNote, int /*sampleOffset*/)
{
    activeNotes.erase(midiNote);
    // Drums don't respond to note off - they decay naturally
}

void DrumSynth::allNotesOff()
{
    for (auto& pad : pads)
    {
        pad.playing = false;
        pad.envelope = 0.0f;
    }
    activeNotes.clear();
}

//==============================================================================
juce::String DrumSynth::getPadName(int padIndex) const
{
    if (padIndex >= 0 && padIndex < NUM_PADS)
        return pads[padIndex].name;
    return "";
}

void DrumSynth::setPadParameter(int padIndex, const juce::String& paramName, float value)
{
    if (padIndex < 0 || padIndex >= NUM_PADS)
        return;

    auto& pad = pads[padIndex];

    if (paramName == "pitch")
        pad.pitch = juce::jlimit(0.5f, 2.0f, value);
    else if (paramName == "decay")
        pad.decay = juce::jlimit(0.0f, 1.0f, value);
    else if (paramName == "tone")
        pad.tone = juce::jlimit(0.0f, 1.0f, value);
    else if (paramName == "level")
        pad.level = juce::jlimit(0.0f, 1.0f, value);
    else if (paramName == "pan")
        pad.pan = juce::jlimit(-1.0f, 1.0f, value);
}

float DrumSynth::getPadParameter(int padIndex, const juce::String& paramName) const
{
    if (padIndex < 0 || padIndex >= NUM_PADS)
        return 0.0f;

    const auto& pad = pads[padIndex];

    if (paramName == "pitch")
        return pad.pitch;
    else if (paramName == "decay")
        return pad.decay;
    else if (paramName == "tone")
        return pad.tone;
    else if (paramName == "level")
        return pad.level;
    else if (paramName == "pan")
        return pad.pan;

    return 0.0f;
}

bool DrumSynth::isNoteActive(int midiNote) const
{
    int padIndex = findPadByNote(midiNote);
    if (padIndex < 0)
        return false;

    return pads[padIndex].playing && pads[padIndex].envelope > 0.0001f;
}

//==============================================================================
void DrumSynth::loadKit(const juce::String& kitName)
{
    currentKit = kitName;

    if (kitName == "808")
        configure808Kit();
    else if (kitName == "909")
        configure909Kit();
    else if (kitName == "Acoustic")
        configureAcousticKit();
    else if (kitName == "Lo-Fi")
        configureLoFiKit();
    else if (kitName == "Trap")
        configureTrapKit();
}

juce::StringArray DrumSynth::getAvailableKits() const
{
    return {"808", "909", "Acoustic", "Lo-Fi", "Trap"};
}

//==============================================================================
std::vector<SynthPreset> DrumSynth::getPresets() const
{
    std::vector<SynthPreset> presets;

    auto kits = getAvailableKits();
    for (int i = 0; i < kits.size(); ++i)
    {
        SynthPreset preset;
        preset.name = kits[i];
        preset.category = "Drums";
        preset.values["volume"] = 0.8f;
        preset.enumValues["kit"] = i;  // Store kit index
        presets.push_back(preset);
    }

    return presets;
}

void DrumSynth::onParameterChanged(const juce::String& /*name*/, float /*value*/)
{
    // Master parameters handled directly
}

void DrumSynth::onParameterEnumChanged(const juce::String& name, int index)
{
    if (name == "kit")
    {
        auto kits = getAvailableKits();
        if (index >= 0 && index < kits.size())
        {
            loadKit(kits[index]);
        }
    }
}

//==============================================================================
// Synthesis implementations
//==============================================================================

float DrumSynth::synthesizeKick(DrumPad& pad, float velocity)
{
    // 808-style kick: sine wave with pitch envelope + click
    const float baseFreq = 55.0f * pad.pitch;
    const float decayTime = 0.1f + pad.decay * 0.5f;
    const float decayRate = 1.0f / (decayTime * static_cast<float>(sampleRateLocal));

    // Pitch envelope (drops from high to low)
    float pitchEnv = std::exp(-pad.phase * 50.0f);
    float freq = baseFreq + pitchEnv * baseFreq * 4.0f * pad.tone;

    // Main oscillator
    float osc = std::sin(pad.phase * freq * juce::MathConstants<float>::twoPi);

    // Click transient
    pad.clickEnv *= 0.99f;
    float click = (noiseRandom.nextFloat() * 2.0f - 1.0f) * pad.clickEnv * pad.tone;

    // Combine
    float output = (osc + click * 0.3f) * pad.envelope * velocity;

    // Update state
    pad.phase += 1.0f / static_cast<float>(sampleRateLocal);
    pad.envelope -= decayRate;
    if (pad.envelope < 0) pad.envelope = 0;

    return output;
}

float DrumSynth::synthesizeSnare(DrumPad& pad, float velocity)
{
    // Snare: pitched body + noise
    const float baseFreq = 180.0f * pad.pitch;
    const float decayTime = 0.05f + pad.decay * 0.2f;
    const float decayRate = 1.0f / (decayTime * static_cast<float>(sampleRateLocal));

    // Body (pitched component)
    float body = std::sin(pad.phase * baseFreq * juce::MathConstants<float>::twoPi);
    body *= std::exp(-pad.phase * 30.0f);

    // Noise (snares)
    float noise = noiseRandom.nextFloat() * 2.0f - 1.0f;
    float noiseEnv = std::exp(-pad.phase * (10.0f + (1.0f - pad.tone) * 20.0f));

    // Mix based on tone
    float mix = body * (1.0f - pad.tone * 0.5f) + noise * noiseEnv * (0.5f + pad.tone * 0.5f);

    float output = mix * pad.envelope * velocity;

    // Update state
    pad.phase += 1.0f / static_cast<float>(sampleRateLocal);
    pad.envelope -= decayRate;
    if (pad.envelope < 0) pad.envelope = 0;

    return output;
}

float DrumSynth::synthesizeHiHat(DrumPad& pad, float velocity, bool open)
{
    // Hi-hat: filtered noise
    const float decayTime = open ? (0.2f + pad.decay * 0.5f) : (0.02f + pad.decay * 0.1f);
    const float decayRate = 1.0f / (decayTime * static_cast<float>(sampleRateLocal));

    // Metallic noise (multiple high frequencies)
    float noise = 0.0f;
    float freq1 = 4000.0f * pad.pitch;
    float freq2 = 6000.0f * pad.pitch;
    float freq3 = 8000.0f * pad.pitch;

    noise += std::sin(pad.phase * freq1 * juce::MathConstants<float>::twoPi) * 0.3f;
    noise += std::sin(pad.phase * freq2 * juce::MathConstants<float>::twoPi) * 0.3f;
    noise += std::sin(pad.phase * freq3 * juce::MathConstants<float>::twoPi) * 0.3f;
    noise += (noiseRandom.nextFloat() * 2.0f - 1.0f) * 0.7f;

    // High-pass filter simulation
    noise *= pad.tone * 0.5f + 0.5f;

    float output = noise * pad.envelope * velocity * 0.5f;

    // Update state
    pad.phase += 1.0f / static_cast<float>(sampleRateLocal);
    pad.envelope -= decayRate;
    if (pad.envelope < 0) pad.envelope = 0;

    return output;
}

float DrumSynth::synthesizeTom(DrumPad& pad, float velocity)
{
    // Tom: resonant drum body with pitch drop and stick attack
    const float baseFreq = 90.0f * pad.pitch;  // Slightly lower for fuller sound
    const float decayTime = 0.15f + pad.decay * 0.5f;
    const float decayRate = 1.0f / (decayTime * static_cast<float>(sampleRateLocal));

    // Pitch envelope - steeper initial drop, settles to base frequency
    float pitchEnv = std::exp(-pad.phase * 35.0f);
    float freq = baseFreq + pitchEnv * baseFreq * 3.0f;

    // Main body oscillator
    float osc = std::sin(pad.phase * freq * juce::MathConstants<float>::twoPi);

    // Sub-harmonic for low toms (adds weight when pitch < 1.0)
    float subWeight = std::max(0.0f, 1.0f - pad.pitch);
    float subOsc = std::sin(pad.phase * freq * 0.5f * juce::MathConstants<float>::twoPi) * subWeight * 0.4f;

    // Second harmonic for body resonance
    float bodyEnv = std::exp(-pad.phase * 15.0f);
    float harmonic = std::sin(pad.phase * freq * 2.1f * juce::MathConstants<float>::twoPi) * bodyEnv * 0.15f;

    // Stick attack (sharp transient with some noise)
    float attackEnv = std::exp(-pad.phase * 150.0f);
    float attack = (noiseRandom.nextFloat() * 2.0f - 1.0f) * attackEnv;
    // Add a pitched click component to the attack
    float clickFreq = 1500.0f * pad.pitch;
    attack += std::sin(pad.phase * clickFreq * juce::MathConstants<float>::twoPi) * attackEnv * 0.3f;

    // Combine all components
    float output = (osc + subOsc + harmonic + attack * 0.25f * pad.tone) * pad.envelope * velocity * 1.1f;

    // Update state
    pad.phase += 1.0f / static_cast<float>(sampleRateLocal);
    pad.envelope -= decayRate;
    if (pad.envelope < 0) pad.envelope = 0;

    return output;
}

float DrumSynth::synthesizeClap(DrumPad& pad, float velocity)
{
    // Clap: multiple hands with staggered timing, bandpassed noise with reverb tail
    const float decayTime = 0.15f + pad.decay * 0.3f;
    const float decayRate = 1.0f / (decayTime * static_cast<float>(sampleRateLocal));

    // Generate noise
    float noise = noiseRandom.nextFloat() * 2.0f - 1.0f;

    // Simulate bandpass filter (800-2500 Hz range for clap character)
    // Use multiple resonant frequencies
    float bpFreq1 = 1000.0f * pad.pitch;
    float bpFreq2 = 1800.0f * pad.pitch;
    float bpFreq3 = 2500.0f * pad.pitch;
    float resonance = std::sin(pad.phase * bpFreq1 * juce::MathConstants<float>::twoPi) * 0.15f;
    resonance += std::sin(pad.phase * bpFreq2 * juce::MathConstants<float>::twoPi) * 0.1f;
    resonance += std::sin(pad.phase * bpFreq3 * juce::MathConstants<float>::twoPi) * 0.05f;

    // Blend noise with resonance for bandpassed character
    float filtered = noise * 0.7f + resonance * noise * (0.5f + pad.tone * 0.5f);

    // Multiple staggered bursts (simulating multiple hands)
    // More realistic timing with slight randomization baked into the pattern
    float t = pad.phase * static_cast<float>(sampleRateLocal);
    float burstEnv = 0.0f;

    // First burst (initial impact)
    if (t < 25) burstEnv = 1.0f;
    // Small gap
    else if (t < 40) burstEnv = 0.1f;
    // Second burst (slightly quieter)
    else if (t < 65) burstEnv = 0.85f;
    // Gap
    else if (t < 85) burstEnv = 0.05f;
    // Third burst
    else if (t < 115) burstEnv = 0.65f;
    // Gap
    else if (t < 140) burstEnv = 0.02f;
    // Fourth burst (weakest)
    else if (t < 175) burstEnv = 0.4f;
    // Reverb-like tail (gradual, uses main envelope)
    else burstEnv = 0.0f;

    // Combine burst attack with sustained tail
    float attack = filtered * burstEnv;
    float tail = filtered * pad.envelope * 0.3f;

    float output = (attack * 0.7f + tail) * velocity * 1.2f;

    // Update state
    pad.phase += 1.0f / static_cast<float>(sampleRateLocal);
    pad.envelope -= decayRate;
    if (pad.envelope < 0) pad.envelope = 0;

    return output;
}

float DrumSynth::synthesizeRim(DrumPad& pad, float velocity)
{
    // Rimshot: sharp transient with resonant wood/metal body
    const float decayTime = 0.04f + pad.decay * 0.12f;
    const float decayRate = 1.0f / (decayTime * static_cast<float>(sampleRateLocal));

    // Multiple resonant modes for realistic rim character
    // Wood body modes (lower frequencies)
    float freq1 = 750.0f * pad.pitch;
    float freq2 = 1100.0f * pad.pitch;
    // Metal ring modes (higher frequencies)
    float freq3 = 1800.0f * pad.pitch;
    float freq4 = 2400.0f * pad.pitch;
    float freq5 = 3200.0f * pad.pitch;

    // Body resonance (decays slower)
    float bodyEnv = std::exp(-pad.phase * 40.0f);
    float body = std::sin(pad.phase * freq1 * juce::MathConstants<float>::twoPi) * 0.35f;
    body += std::sin(pad.phase * freq2 * juce::MathConstants<float>::twoPi) * 0.25f;
    body *= bodyEnv;

    // Metal ring (decays faster, higher frequencies)
    float ringEnv = std::exp(-pad.phase * 80.0f);
    float ring = std::sin(pad.phase * freq3 * juce::MathConstants<float>::twoPi) * 0.2f;
    ring += std::sin(pad.phase * freq4 * juce::MathConstants<float>::twoPi) * 0.12f;
    ring += std::sin(pad.phase * freq5 * juce::MathConstants<float>::twoPi) * 0.08f;
    ring *= ringEnv * (0.5f + pad.tone * 0.5f);  // Tone controls ring amount

    // Sharp transient click
    float clickEnv = std::exp(-pad.phase * 300.0f);
    float click = (noiseRandom.nextFloat() * 2.0f - 1.0f) * clickEnv * 0.5f;

    // Combine with body/ring balance based on tone
    float output = (body * (1.0f - pad.tone * 0.3f) + ring + click) * pad.envelope * velocity * 1.3f;

    // Update state
    pad.phase += 1.0f / static_cast<float>(sampleRateLocal);
    pad.envelope -= decayRate;
    if (pad.envelope < 0) pad.envelope = 0;

    return output;
}

float DrumSynth::synthesizeCymbal(DrumPad& pad, float velocity)
{
    // Cymbal: metallic character with bell modes and sizzle
    // tone controls bell vs wash: low tone = crash (more wash), high tone = ride (more bell)
    const float decayTime = 0.6f + pad.decay * 2.5f;
    const float decayRate = 1.0f / (decayTime * static_cast<float>(sampleRateLocal));

    // Inharmonic frequency ratios typical of cymbals (based on bronze modal analysis)
    // These ratios create the characteristic metallic timbre
    const float baseFreq = 400.0f * pad.pitch;
    const float ratios[] = {1.0f, 1.47f, 1.65f, 2.32f, 2.56f, 3.12f, 3.87f, 4.15f, 5.23f, 6.71f};

    // Bell component (defined pitch, decays slower)
    float bellEnv = std::exp(-pad.phase * (8.0f + (1.0f - pad.tone) * 15.0f));
    float bell = 0.0f;
    bell += std::sin(pad.phase * baseFreq * ratios[0] * juce::MathConstants<float>::twoPi) * 0.25f;
    bell += std::sin(pad.phase * baseFreq * ratios[1] * juce::MathConstants<float>::twoPi) * 0.2f;
    bell += std::sin(pad.phase * baseFreq * ratios[2] * juce::MathConstants<float>::twoPi) * 0.15f;
    bell *= bellEnv * (0.3f + pad.tone * 0.7f);  // More bell with higher tone

    // Body/wash component (noisier, characteristic shimmer)
    float bodyEnv = std::exp(-pad.phase * (3.0f + pad.tone * 5.0f));
    float body = 0.0f;
    // Higher partials create the wash/shimmer
    body += std::sin(pad.phase * baseFreq * ratios[3] * juce::MathConstants<float>::twoPi) * 0.12f;
    body += std::sin(pad.phase * baseFreq * ratios[4] * juce::MathConstants<float>::twoPi) * 0.1f;
    body += std::sin(pad.phase * baseFreq * ratios[5] * juce::MathConstants<float>::twoPi) * 0.08f;
    body += std::sin(pad.phase * baseFreq * ratios[6] * juce::MathConstants<float>::twoPi) * 0.06f;
    body += std::sin(pad.phase * baseFreq * ratios[7] * juce::MathConstants<float>::twoPi) * 0.05f;
    body += std::sin(pad.phase * baseFreq * ratios[8] * juce::MathConstants<float>::twoPi) * 0.04f;
    body += std::sin(pad.phase * baseFreq * ratios[9] * juce::MathConstants<float>::twoPi) * 0.03f;
    body *= bodyEnv * (0.7f - pad.tone * 0.3f);  // More wash with lower tone

    // Sizzle/noise component for high frequency content
    float sizzle = (noiseRandom.nextFloat() * 2.0f - 1.0f);
    float sizzleEnv = std::exp(-pad.phase * (2.0f + pad.tone * 3.0f));
    sizzle *= sizzleEnv * 0.25f;

    // Attack transient (stick hit)
    float attackEnv = std::exp(-pad.phase * 200.0f);
    float attack = (noiseRandom.nextFloat() * 2.0f - 1.0f) * attackEnv * 0.3f;

    // Combine all components
    float output = (bell + body + sizzle + attack) * pad.envelope * velocity * 0.9f;

    // Update state
    pad.phase += 1.0f / static_cast<float>(sampleRateLocal);
    pad.envelope -= decayRate;
    if (pad.envelope < 0) pad.envelope = 0;

    return output;
}

float DrumSynth::synthesizeCowbell(DrumPad& pad, float velocity)
{
    // Cowbell: two inharmonic tones
    const float decayTime = 0.1f + pad.decay * 0.3f;
    const float decayRate = 1.0f / (decayTime * static_cast<float>(sampleRateLocal));

    float freq1 = 560.0f * pad.pitch;
    float freq2 = 845.0f * pad.pitch;

    float osc1 = std::sin(pad.phase * freq1 * juce::MathConstants<float>::twoPi);
    float osc2 = std::sin(pad.phase * freq2 * juce::MathConstants<float>::twoPi);

    float output = (osc1 * 0.5f + osc2 * 0.5f) * pad.envelope * velocity;

    // Update state
    pad.phase += 1.0f / static_cast<float>(sampleRateLocal);
    pad.envelope -= decayRate;
    if (pad.envelope < 0) pad.envelope = 0;

    return output;
}

float DrumSynth::synthesizeClave(DrumPad& pad, float velocity)
{
    // Clave: short woody click
    const float decayTime = 0.02f + pad.decay * 0.05f;
    const float decayRate = 1.0f / (decayTime * static_cast<float>(sampleRateLocal));

    float freq = 2500.0f * pad.pitch;
    float osc = std::sin(pad.phase * freq * juce::MathConstants<float>::twoPi);

    float output = osc * pad.envelope * velocity;

    // Update state
    pad.phase += 1.0f / static_cast<float>(sampleRateLocal);
    pad.envelope -= decayRate;
    if (pad.envelope < 0) pad.envelope = 0;

    return output;
}

float DrumSynth::synthesizeShaker(DrumPad& pad, float velocity)
{
    // Shaker: grainy filtered noise with amplitude modulation for bead texture
    const float decayTime = 0.1f + pad.decay * 0.25f;
    const float decayRate = 1.0f / (decayTime * static_cast<float>(sampleRateLocal));

    // Base noise
    float noise = noiseRandom.nextFloat() * 2.0f - 1.0f;

    // Amplitude modulation to create "grains" (simulates individual beads hitting)
    // Multiple LFO rates create complex rhythmic texture
    float grainRate1 = 120.0f + pad.pitch * 60.0f;  // Primary grain rate
    float grainRate2 = 180.0f + pad.pitch * 90.0f;  // Secondary grain rate
    float grainRate3 = 280.0f + pad.pitch * 50.0f;  // Tertiary for complexity

    float grain1 = std::sin(pad.phase * grainRate1 * juce::MathConstants<float>::twoPi);
    float grain2 = std::sin(pad.phase * grainRate2 * juce::MathConstants<float>::twoPi);
    float grain3 = std::sin(pad.phase * grainRate3 * juce::MathConstants<float>::twoPi);

    // Combine grains into modulation envelope
    float grainEnv = 0.5f + 0.2f * grain1 + 0.15f * grain2 + 0.1f * grain3;
    grainEnv = std::max(0.1f, grainEnv);  // Prevent complete silence

    // High frequency resonance for "container" character
    float containerFreq1 = 6000.0f * pad.pitch;
    float containerFreq2 = 9000.0f * pad.pitch;
    float container = std::sin(pad.phase * containerFreq1 * juce::MathConstants<float>::twoPi) * 0.1f;
    container += std::sin(pad.phase * containerFreq2 * juce::MathConstants<float>::twoPi) * 0.05f;

    // Mix noise with grain modulation
    float texturedNoise = noise * grainEnv;

    // Blend noise and container resonance based on tone
    float output = (texturedNoise * (0.7f + pad.tone * 0.3f) + container * pad.tone);

    // Apply overall envelope and velocity
    output *= pad.envelope * velocity * 0.85f;

    // Update state
    pad.phase += 1.0f / static_cast<float>(sampleRateLocal);
    pad.envelope -= decayRate;
    if (pad.envelope < 0) pad.envelope = 0;

    return output;
}

float DrumSynth::synthesizeConga(DrumPad& pad, float velocity)
{
    // Conga: resonant skin drum with warm body and slap attack
    const float decayTime = 0.12f + pad.decay * 0.35f;
    const float decayRate = 1.0f / (decayTime * static_cast<float>(sampleRateLocal));

    // Base frequency with pitch bend (congas have characteristic pitch drop)
    const float baseFreq = 200.0f * pad.pitch;
    float pitchEnv = std::exp(-pad.phase * 25.0f);
    float freq = baseFreq + pitchEnv * baseFreq * 0.8f;

    // Primary drum body (skin resonance)
    float body = std::sin(pad.phase * freq * juce::MathConstants<float>::twoPi);

    // Second mode (slightly inharmonic for realistic drum character)
    float mode2Freq = freq * 1.58f;  // Typical drum membrane ratio
    float mode2 = std::sin(pad.phase * mode2Freq * juce::MathConstants<float>::twoPi) * 0.3f;

    // Third mode (adds brightness)
    float mode3Freq = freq * 2.14f;
    float mode3Env = std::exp(-pad.phase * 40.0f);
    float mode3 = std::sin(pad.phase * mode3Freq * juce::MathConstants<float>::twoPi) * mode3Env * 0.15f;

    // Slap attack (hand hitting skin)
    float slapEnv = std::exp(-pad.phase * 180.0f);
    float slap = (noiseRandom.nextFloat() * 2.0f - 1.0f) * slapEnv;
    // Add some high frequency content to the slap
    float slapTone = std::sin(pad.phase * 1800.0f * pad.pitch * juce::MathConstants<float>::twoPi);
    slap = slap * 0.6f + slapTone * slapEnv * 0.4f;

    // Body resonance envelope (decays slower than attack)
    float bodyEnv = std::exp(-pad.phase * 12.0f);

    // Combine components - tone controls slap vs body balance
    float tonal = (body + mode2 + mode3) * bodyEnv;
    float output = (tonal * (0.7f + (1.0f - pad.tone) * 0.3f) + slap * 0.35f * pad.tone);
    output *= pad.envelope * velocity * 1.1f;

    // Update state
    pad.phase += 1.0f / static_cast<float>(sampleRateLocal);
    pad.envelope -= decayRate;
    if (pad.envelope < 0) pad.envelope = 0;

    return output;
}

//==============================================================================
int DrumSynth::findPadByNote(int midiNote) const
{
    for (int i = 0; i < NUM_PADS; ++i)
    {
        if (pads[i].midiNote == midiNote)
            return i;
    }
    return -1;
}

void DrumSynth::chokePadsInGroup(int chokeGroup, int exceptPad)
{
    for (int i = 0; i < NUM_PADS; ++i)
    {
        if (i != exceptPad && pads[i].chokeGroup == chokeGroup)
        {
            pads[i].playing = false;
            pads[i].envelope = 0.0f;
        }
    }
}

//==============================================================================
// Kit configurations
//==============================================================================

void DrumSynth::configure808Kit()
{
    // 808: Long boomy kick, punchy snare, classic electronic
    pads[0].pitch = 1.0f;  pads[0].decay = 0.7f;  pads[0].tone = 0.3f;  // Kick
    pads[1].pitch = 1.0f;  pads[1].decay = 0.4f;  pads[1].tone = 0.5f;  // Snare
    pads[2].pitch = 1.0f;  pads[2].decay = 0.2f;  pads[2].tone = 0.8f;  // Closed HH
    pads[3].pitch = 1.0f;  pads[3].decay = 0.6f;  pads[3].tone = 0.8f;  // Open HH
    pads[4].pitch = 0.8f;  pads[4].decay = 0.5f;  pads[4].tone = 0.4f;  // Tom Low
    pads[5].pitch = 1.0f;  pads[5].decay = 0.45f; pads[5].tone = 0.5f;  // Tom Mid
    pads[6].pitch = 1.3f;  pads[6].decay = 0.4f;  pads[6].tone = 0.6f;  // Tom High
    pads[7].pitch = 1.0f;  pads[7].decay = 0.5f;  pads[7].tone = 0.6f;  // Clap
    pads[8].pitch = 1.0f;  pads[8].decay = 0.3f;  pads[8].tone = 0.7f;  // Rim
    pads[9].pitch = 1.0f;  pads[9].decay = 0.7f;  pads[9].tone = 0.7f;  // Crash
    pads[10].pitch = 1.0f; pads[10].decay = 0.6f; pads[10].tone = 0.8f; // Ride
    pads[11].pitch = 1.0f; pads[11].decay = 0.4f; pads[11].tone = 0.5f; // Cowbell
    pads[12].pitch = 1.0f; pads[12].decay = 0.3f; pads[12].tone = 0.5f; // Clave
    pads[13].pitch = 1.0f; pads[13].decay = 0.4f; pads[13].tone = 0.7f; // Shaker
    pads[14].pitch = 1.2f; pads[14].decay = 0.3f; pads[14].tone = 0.6f; // Perc 1
    pads[15].pitch = 0.9f; pads[15].decay = 0.35f; pads[15].tone = 0.5f; // Perc 2
}

void DrumSynth::configure909Kit()
{
    // 909: Punchier, more attack, classic house/techno
    pads[0].pitch = 1.1f;  pads[0].decay = 0.5f;  pads[0].tone = 0.5f;  // Kick
    pads[1].pitch = 1.0f;  pads[1].decay = 0.3f;  pads[1].tone = 0.7f;  // Snare
    pads[2].pitch = 1.2f;  pads[2].decay = 0.15f; pads[2].tone = 0.9f;  // Closed HH
    pads[3].pitch = 1.2f;  pads[3].decay = 0.5f;  pads[3].tone = 0.9f;  // Open HH
    pads[4].pitch = 0.9f;  pads[4].decay = 0.4f;  pads[4].tone = 0.5f;  // Tom Low
    pads[5].pitch = 1.1f;  pads[5].decay = 0.35f; pads[5].tone = 0.6f;  // Tom Mid
    pads[6].pitch = 1.4f;  pads[6].decay = 0.3f;  pads[6].tone = 0.7f;  // Tom High
    pads[7].pitch = 1.1f;  pads[7].decay = 0.4f;  pads[7].tone = 0.7f;  // Clap
    pads[8].pitch = 1.2f;  pads[8].decay = 0.25f; pads[8].tone = 0.8f;  // Rim
    pads[9].pitch = 1.1f;  pads[9].decay = 0.8f;  pads[9].tone = 0.8f;  // Crash
    pads[10].pitch = 1.15f; pads[10].decay = 0.7f; pads[10].tone = 0.85f; // Ride
    pads[11].pitch = 1.1f; pads[11].decay = 0.35f; pads[11].tone = 0.6f; // Cowbell
    pads[12].pitch = 1.1f; pads[12].decay = 0.25f; pads[12].tone = 0.6f; // Clave
    pads[13].pitch = 1.1f; pads[13].decay = 0.35f; pads[13].tone = 0.8f; // Shaker
    pads[14].pitch = 1.3f; pads[14].decay = 0.25f; pads[14].tone = 0.7f; // Perc 1
    pads[15].pitch = 1.0f; pads[15].decay = 0.3f; pads[15].tone = 0.6f;  // Perc 2
}

void DrumSynth::configureAcousticKit()
{
    // Acoustic: Natural, roomy, realistic
    pads[0].pitch = 0.8f;  pads[0].decay = 0.4f;  pads[0].tone = 0.4f;  // Kick
    pads[1].pitch = 1.0f;  pads[1].decay = 0.35f; pads[1].tone = 0.6f;  // Snare
    pads[2].pitch = 1.1f;  pads[2].decay = 0.1f;  pads[2].tone = 0.7f;  // Closed HH
    pads[3].pitch = 1.0f;  pads[3].decay = 0.4f;  pads[3].tone = 0.7f;  // Open HH
    pads[4].pitch = 0.7f;  pads[4].decay = 0.5f;  pads[4].tone = 0.4f;  // Tom Low
    pads[5].pitch = 0.9f;  pads[5].decay = 0.45f; pads[5].tone = 0.5f;  // Tom Mid
    pads[6].pitch = 1.2f;  pads[6].decay = 0.4f;  pads[6].tone = 0.55f; // Tom High
    pads[7].pitch = 0.95f; pads[7].decay = 0.4f;  pads[7].tone = 0.5f;  // Clap
    pads[8].pitch = 1.1f;  pads[8].decay = 0.2f;  pads[8].tone = 0.6f;  // Rim
    pads[9].pitch = 0.9f;  pads[9].decay = 1.0f;  pads[9].tone = 0.6f;  // Crash
    pads[10].pitch = 0.95f; pads[10].decay = 0.8f; pads[10].tone = 0.7f; // Ride
    pads[11].pitch = 1.0f; pads[11].decay = 0.3f; pads[11].tone = 0.5f; // Cowbell
    pads[12].pitch = 1.0f; pads[12].decay = 0.2f; pads[12].tone = 0.5f; // Clave
    pads[13].pitch = 0.9f; pads[13].decay = 0.3f; pads[13].tone = 0.6f; // Shaker
    pads[14].pitch = 1.1f; pads[14].decay = 0.25f; pads[14].tone = 0.55f; // Perc 1
    pads[15].pitch = 0.85f; pads[15].decay = 0.3f; pads[15].tone = 0.5f; // Perc 2
}

void DrumSynth::configureLoFiKit()
{
    // Lo-Fi: Crunchy, vintage, dusty
    pads[0].pitch = 0.9f;  pads[0].decay = 0.5f;  pads[0].tone = 0.6f;  // Kick
    pads[1].pitch = 0.95f; pads[1].decay = 0.25f; pads[1].tone = 0.8f;  // Snare
    pads[2].pitch = 0.9f;  pads[2].decay = 0.1f;  pads[2].tone = 0.6f;  // Closed HH
    pads[3].pitch = 0.9f;  pads[3].decay = 0.3f;  pads[3].tone = 0.6f;  // Open HH
    pads[4].pitch = 0.75f; pads[4].decay = 0.4f;  pads[4].tone = 0.5f;  // Tom Low
    pads[5].pitch = 0.85f; pads[5].decay = 0.35f; pads[5].tone = 0.55f; // Tom Mid
    pads[6].pitch = 1.0f;  pads[6].decay = 0.3f;  pads[6].tone = 0.6f;  // Tom High
    pads[7].pitch = 0.9f;  pads[7].decay = 0.35f; pads[7].tone = 0.7f;  // Clap
    pads[8].pitch = 0.95f; pads[8].decay = 0.2f;  pads[8].tone = 0.65f; // Rim
    pads[9].pitch = 0.85f; pads[9].decay = 0.6f;  pads[9].tone = 0.55f; // Crash
    pads[10].pitch = 0.88f; pads[10].decay = 0.5f; pads[10].tone = 0.6f; // Ride
    pads[11].pitch = 0.9f; pads[11].decay = 0.3f; pads[11].tone = 0.5f; // Cowbell
    pads[12].pitch = 0.9f; pads[12].decay = 0.2f; pads[12].tone = 0.5f; // Clave
    pads[13].pitch = 0.85f; pads[13].decay = 0.25f; pads[13].tone = 0.55f; // Shaker
    pads[14].pitch = 0.95f; pads[14].decay = 0.22f; pads[14].tone = 0.55f; // Perc 1
    pads[15].pitch = 0.8f; pads[15].decay = 0.28f; pads[15].tone = 0.5f; // Perc 2
}

void DrumSynth::configureTrapKit()
{
    // Trap: Deep 808, sharp snare, hard-hitting
    pads[0].pitch = 0.7f;  pads[0].decay = 0.9f;  pads[0].tone = 0.2f;  // Kick - deep
    pads[1].pitch = 1.2f;  pads[1].decay = 0.2f;  pads[1].tone = 0.9f;  // Snare - crispy
    pads[2].pitch = 1.3f;  pads[2].decay = 0.1f;  pads[2].tone = 1.0f;  // Closed HH - bright
    pads[3].pitch = 1.3f;  pads[3].decay = 0.4f;  pads[3].tone = 1.0f;  // Open HH
    pads[4].pitch = 0.6f;  pads[4].decay = 0.7f;  pads[4].tone = 0.3f;  // Tom Low - deep
    pads[5].pitch = 0.8f;  pads[5].decay = 0.55f; pads[5].tone = 0.4f;  // Tom Mid
    pads[6].pitch = 1.1f;  pads[6].decay = 0.4f;  pads[6].tone = 0.5f;  // Tom High
    pads[7].pitch = 1.15f; pads[7].decay = 0.35f; pads[7].tone = 0.85f; // Clap - snappy
    pads[8].pitch = 1.3f;  pads[8].decay = 0.15f; pads[8].tone = 0.9f;  // Rim - tight
    pads[9].pitch = 1.2f;  pads[9].decay = 0.9f;  pads[9].tone = 0.85f; // Crash
    pads[10].pitch = 1.25f; pads[10].decay = 0.7f; pads[10].tone = 0.9f; // Ride
    pads[11].pitch = 1.15f; pads[11].decay = 0.3f; pads[11].tone = 0.6f; // Cowbell
    pads[12].pitch = 1.2f; pads[12].decay = 0.2f; pads[12].tone = 0.7f; // Clave
    pads[13].pitch = 1.2f; pads[13].decay = 0.3f; pads[13].tone = 0.85f; // Shaker
    pads[14].pitch = 1.4f; pads[14].decay = 0.2f; pads[14].tone = 0.8f;  // Perc 1
    pads[15].pitch = 1.1f; pads[15].decay = 0.25f; pads[15].tone = 0.65f; // Perc 2
}
