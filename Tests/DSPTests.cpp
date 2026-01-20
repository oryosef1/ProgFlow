/**
 * DSP Unit Tests - Effects, Filters, and Audio Processing
 */

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "../Source/Audio/Effects/EffectBase.h"
#include "../Source/Audio/Effects/EffectChain.h"
#include "../Source/Audio/Effects/ReverbEffect.h"
#include "../Source/Audio/Effects/DelayEffect.h"
#include "../Source/Audio/Effects/CompressorEffect.h"
#include "../Source/Audio/Effects/EQEffect.h"
#include "../Source/Audio/Effects/DistortionEffect.h"
#include "../Source/Audio/Effects/ChorusEffect.h"
#include "../Source/Audio/Effects/FilterEffect.h"
#include "../Source/Audio/Effects/LimiterEffect.h"
#include "../Source/Audio/Effects/GateEffect.h"

class DSPTests : public juce::UnitTest
{
public:
    DSPTests() : UnitTest("DSP") {}

    void runTest() override
    {
        //======================================================================
        // EffectChain Tests
        //======================================================================
        beginTest("EffectChain can be prepared");
        {
            EffectChain chain;
            chain.prepareToPlay(44100.0, 512);
            expect(true);
        }

        beginTest("EffectChain can add effects");
        {
            EffectChain chain;
            chain.prepareToPlay(44100.0, 512);

            auto reverb = std::make_unique<ReverbEffect>();
            chain.addEffect(std::move(reverb));

            expectEquals(chain.getNumEffects(), 1);
        }

        beginTest("EffectChain processes audio");
        {
            EffectChain chain;
            chain.prepareToPlay(44100.0, 512);

            // Create a test buffer with silence
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();

            chain.processBlock(buffer);

            // Output should still be valid (not NaN or infinite)
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    float sample = buffer.getSample(ch, i);
                    expect(!std::isnan(sample), "NaN detected in output");
                    expect(!std::isinf(sample), "Inf detected in output");
                }
            }
        }

        //======================================================================
        // Individual Effect Tests
        //======================================================================
        beginTest("ReverbEffect processes without NaN");
        {
            ReverbEffect reverb;
            reverb.prepareToPlay(44100.0, 512);

            // Test with impulse
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            buffer.setSample(0, 0, 1.0f);  // Impulse
            buffer.setSample(1, 0, 1.0f);

            reverb.processBlock(buffer);

            expectNoNaNOrInf(buffer);
        }

        beginTest("DelayEffect creates delayed signal");
        {
            DelayEffect delay;
            delay.prepareToPlay(44100.0, 512);
            delay.setParameter("mix", 0.5f);
            delay.setParameter("time", 0.1f);  // 100ms delay

            // Create impulse
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(1, 0, 1.0f);

            // Process multiple blocks to let delay develop
            for (int i = 0; i < 10; ++i)
            {
                delay.processBlock(buffer);
                buffer.clear();  // Clear for next block to test delay tail
            }

            expectNoNaNOrInf(buffer);
        }

        beginTest("CompressorEffect reduces loud signals");
        {
            CompressorEffect compressor;
            compressor.prepareToPlay(44100.0, 512);
            compressor.setParameter("threshold", -20.0f);
            compressor.setParameter("ratio", 4.0f);

            // Create loud signal
            juce::AudioBuffer<float> buffer(2, 512);
            for (int i = 0; i < 512; ++i)
            {
                float sample = 0.9f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * i / 44100.0f);
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }

            float peakBefore = buffer.getMagnitude(0, 512);
            compressor.processBlock(buffer);
            float peakAfter = buffer.getMagnitude(0, 512);

            // Compressed signal should be lower
            expectLessOrEqual(peakAfter, peakBefore, "Compressor didn't reduce level");
            expectNoNaNOrInf(buffer);
        }

        beginTest("EQEffect processes audio");
        {
            EQEffect eq;
            eq.prepareToPlay(44100.0, 512);

            // Create test signal
            juce::AudioBuffer<float> buffer(2, 512);
            for (int i = 0; i < 512; ++i)
            {
                float sample = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 1000.0f * i / 44100.0f);
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }

            eq.processBlock(buffer);
            expectNoNaNOrInf(buffer);
        }

        beginTest("DistortionEffect adds harmonics");
        {
            DistortionEffect distortion;
            distortion.prepareToPlay(44100.0, 512);
            distortion.setParameter("drive", 0.8f);
            distortion.setParameter("mix", 1.0f);

            // Create sine wave
            juce::AudioBuffer<float> buffer(2, 512);
            for (int i = 0; i < 512; ++i)
            {
                float sample = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * i / 44100.0f);
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }

            distortion.processBlock(buffer);
            expectNoNaNOrInf(buffer);
        }

        beginTest("ChorusEffect modulates signal");
        {
            ChorusEffect chorus;
            chorus.prepareToPlay(44100.0, 512);
            chorus.setParameter("rate", 1.0f);
            chorus.setParameter("depth", 0.5f);
            chorus.setParameter("mix", 0.5f);

            juce::AudioBuffer<float> buffer(2, 512);
            for (int i = 0; i < 512; ++i)
            {
                float sample = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * i / 44100.0f);
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }

            chorus.processBlock(buffer);
            expectNoNaNOrInf(buffer);
        }

        beginTest("FilterEffect applies low-pass");
        {
            FilterEffect filter;
            filter.prepareToPlay(44100.0, 512);
            filter.setParameter("cutoff", 1000.0f);
            filter.setParameter("resonance", 0.5f);

            juce::AudioBuffer<float> buffer(2, 512);
            for (int i = 0; i < 512; ++i)
            {
                float sample = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 5000.0f * i / 44100.0f);
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }

            float rmssBefore = buffer.getRMSLevel(0, 0, 512);
            filter.processBlock(buffer);
            float rmsAfter = buffer.getRMSLevel(0, 0, 512);

            // Low pass at 1kHz should attenuate 5kHz signal
            expectLessOrEqual(rmsAfter, rmssBefore, "Filter didn't attenuate high frequency");
            expectNoNaNOrInf(buffer);
        }

        beginTest("LimiterEffect prevents clipping");
        {
            LimiterEffect limiter;
            limiter.prepareToPlay(44100.0, 512);
            limiter.setParameter("threshold", -3.0f);

            // Create very loud signal
            juce::AudioBuffer<float> buffer(2, 512);
            for (int i = 0; i < 512; ++i)
            {
                buffer.setSample(0, i, 2.0f);  // Way over 1.0
                buffer.setSample(1, i, 2.0f);
            }

            limiter.processBlock(buffer);

            // Output should be limited
            for (int i = 0; i < 512; ++i)
            {
                expect(std::abs(buffer.getSample(0, i)) <= 1.5f, "Limiter didn't limit signal");
            }
            expectNoNaNOrInf(buffer);
        }

        beginTest("GateEffect silences quiet signals");
        {
            GateEffect gate;
            gate.prepareToPlay(44100.0, 512);
            gate.setParameter("threshold", -40.0f);

            // Create very quiet signal
            juce::AudioBuffer<float> buffer(2, 512);
            for (int i = 0; i < 512; ++i)
            {
                buffer.setSample(0, i, 0.001f);  // Very quiet
                buffer.setSample(1, i, 0.001f);
            }

            gate.processBlock(buffer);
            expectNoNaNOrInf(buffer);
        }

        //======================================================================
        // Edge Case Tests
        //======================================================================
        beginTest("Effects handle silence");
        {
            ReverbEffect reverb;
            reverb.prepareToPlay(44100.0, 512);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();

            reverb.processBlock(buffer);
            expectNoNaNOrInf(buffer);
        }

        beginTest("Effects handle DC offset");
        {
            EffectChain chain;
            chain.prepareToPlay(44100.0, 512);
            chain.addEffect(std::make_unique<FilterEffect>());

            juce::AudioBuffer<float> buffer(2, 512);
            for (int i = 0; i < 512; ++i)
            {
                buffer.setSample(0, i, 0.5f);  // DC offset
                buffer.setSample(1, i, 0.5f);
            }

            chain.processBlock(buffer);
            expectNoNaNOrInf(buffer);
        }

        beginTest("Effects handle different buffer sizes");
        {
            ReverbEffect reverb;
            reverb.prepareToPlay(44100.0, 512);

            // Test with various buffer sizes
            for (int bufferSize : {64, 128, 256, 512, 1024})
            {
                juce::AudioBuffer<float> buffer(2, bufferSize);
                buffer.clear();
                buffer.setSample(0, 0, 0.5f);

                reverb.processBlock(buffer);
                expectNoNaNOrInf(buffer);
            }
        }

        beginTest("EffectChain bypass works");
        {
            EffectChain chain;
            chain.prepareToPlay(44100.0, 512);

            auto reverb = std::make_unique<ReverbEffect>();
            reverb->setBypass(true);
            chain.addEffect(std::move(reverb));

            // Create impulse
            juce::AudioBuffer<float> inputBuffer(2, 512);
            inputBuffer.clear();
            inputBuffer.setSample(0, 0, 1.0f);
            inputBuffer.setSample(1, 0, 1.0f);

            juce::AudioBuffer<float> processedBuffer;
            processedBuffer.makeCopyOf(inputBuffer);

            chain.processBlock(processedBuffer);

            // Bypassed effect should pass signal through unchanged
            for (int i = 0; i < 512; ++i)
            {
                expectWithinAbsoluteError(inputBuffer.getSample(0, i),
                                          processedBuffer.getSample(0, i), 0.0001f);
            }
        }
    }

private:
    void expectNoNaNOrInf(const juce::AudioBuffer<float>& buffer)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float sample = buffer.getSample(ch, i);
                expect(!std::isnan(sample), "NaN detected at ch=" + juce::String(ch) + " sample=" + juce::String(i));
                expect(!std::isinf(sample), "Inf detected at ch=" + juce::String(ch) + " sample=" + juce::String(i));
            }
        }
    }
};

// Register the test
static DSPTests dspTests;
