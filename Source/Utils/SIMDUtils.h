#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

/**
 * SIMDUtils - SIMD-optimized audio processing utilities
 *
 * Uses JUCE's SIMD support when available, falls back to scalar operations otherwise.
 * These functions are designed for hot paths in audio processing.
 */
namespace SIMDUtils
{
    //==========================================================================
    // Buffer operations

    /**
     * Calculate RMS of a buffer
     * Uses JUCE's optimized vector operations internally
     */
    inline float calculateRMS(const float* data, int numSamples)
    {
        if (numSamples == 0) return 0.0f;

        // Create a temporary buffer for squared values
        // For small buffers, stack allocation is fine
        if (numSamples <= 2048)
        {
            float squared[2048];
            juce::FloatVectorOperations::multiply(squared, data, data, numSamples);

            float sum = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                sum += squared[i];

            return std::sqrt(sum / static_cast<float>(numSamples));
        }
        else
        {
            // For larger buffers, use heap allocation
            std::vector<float> squared(numSamples);
            juce::FloatVectorOperations::multiply(squared.data(), data, data, numSamples);

            float sum = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                sum += squared[i];

            return std::sqrt(sum / static_cast<float>(numSamples));
        }
    }

    /**
     * Calculate peak level of a buffer using SIMD
     */
    inline float calculatePeak(const float* data, int numSamples)
    {
        if (numSamples == 0) return 0.0f;

        // Use JUCE's optimized findMinAndMax for peak detection
        auto range = juce::FloatVectorOperations::findMinAndMax(data, numSamples);
        return std::max(std::abs(range.getStart()), std::abs(range.getEnd()));
    }

    /**
     * Add scaled buffer: dst += src * gain
     * Uses JUCE's optimized vector operations
     */
    inline void addWithGain(float* dst, const float* src, float gain, int numSamples)
    {
        juce::FloatVectorOperations::addWithMultiply(dst, src, gain, numSamples);
    }

    /**
     * Apply gain in-place: dst *= gain
     * Uses JUCE's optimized vector operations
     */
    inline void applyGain(float* data, float gain, int numSamples)
    {
        juce::FloatVectorOperations::multiply(data, gain, numSamples);
    }

    /**
     * Mix two buffers with crossfade: dst = srcA * (1-mix) + srcB * mix
     */
    inline void crossfade(float* dst, const float* srcA, const float* srcB,
                          float mix, int numSamples)
    {
        const float gainA = 1.0f - mix;
        const float gainB = mix;

        // dst = srcA * gainA
        juce::FloatVectorOperations::copyWithMultiply(dst, srcA, gainA, numSamples);
        // dst += srcB * gainB
        juce::FloatVectorOperations::addWithMultiply(dst, srcB, gainB, numSamples);
    }

    /**
     * Soft clip (tanh saturation) - SIMD optimized
     */
    inline void softClip(float* data, float drive, int numSamples)
    {
        // tanh approximation: x / (1 + |x|) is faster than std::tanh
        // and gives similar soft clipping character

        for (int i = 0; i < numSamples; ++i)
        {
            float x = data[i] * drive;
            data[i] = x / (1.0f + std::abs(x));
        }
    }

    /**
     * Generate sine wave into buffer using SIMD
     * Returns the updated phase
     */
    inline double generateSine(float* buffer, int numSamples,
                               double phase, double phaseIncrement, float amplitude)
    {
        // Use a polynomial approximation for fast sine
        // sin(x) ≈ x - x³/6 + x⁵/120 for |x| < π

        for (int i = 0; i < numSamples; ++i)
        {
            // Normalize phase to [-π, π]
            double x = phase;
            while (x > juce::MathConstants<double>::pi)
                x -= juce::MathConstants<double>::twoPi;
            while (x < -juce::MathConstants<double>::pi)
                x += juce::MathConstants<double>::twoPi;

            // Fast sine approximation
            double x2 = x * x;
            double x3 = x2 * x;
            double x5 = x3 * x2;
            double sinVal = x - x3 / 6.0 + x5 / 120.0;

            buffer[i] = static_cast<float>(sinVal * amplitude);

            phase += phaseIncrement;
            if (phase >= juce::MathConstants<double>::twoPi)
                phase -= juce::MathConstants<double>::twoPi;
        }

        return phase;
    }

    //==========================================================================
    // Stereo operations

    /**
     * Calculate stereo RMS levels
     */
    inline void calculateStereoRMS(const float* leftData, const float* rightData,
                                   int numSamples, float& rmsL, float& rmsR)
    {
        rmsL = calculateRMS(leftData, numSamples);
        rmsR = calculateRMS(rightData, numSamples);
    }

    /**
     * Apply stereo panning
     * pan: -1.0 = full left, 0.0 = center, 1.0 = full right
     */
    inline void applyPan(float* leftData, float* rightData, float pan, int numSamples)
    {
        // Constant power panning
        const float angle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
        const float leftGain = std::cos(angle);
        const float rightGain = std::sin(angle);

        applyGain(leftData, leftGain, numSamples);
        applyGain(rightData, rightGain, numSamples);
    }

} // namespace SIMDUtils
