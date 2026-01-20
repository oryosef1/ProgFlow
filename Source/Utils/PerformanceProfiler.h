#pragma once

#include <juce_core/juce_core.h>
#include <atomic>
#include <array>
#include <string>

/**
 * PerformanceProfiler - Lightweight profiler for audio processing hot paths
 *
 * Usage:
 *   PROFILE_SCOPE("AudioEngine::getNextAudioBlock");
 *   // ... code to profile ...
 *
 * Or for manual control:
 *   PerformanceProfiler::getInstance().beginSection("ProcessSynth");
 *   // ... code ...
 *   PerformanceProfiler::getInstance().endSection("ProcessSynth");
 *
 * Statistics are thread-safe and can be read from UI thread.
 */

// Set to 0 to completely disable profiling (zero overhead)
#define PROGFLOW_PROFILING_ENABLED 1

#if PROGFLOW_PROFILING_ENABLED
    #define PROFILE_CONCAT_IMPL(a, b) a##b
    #define PROFILE_CONCAT(a, b) PROFILE_CONCAT_IMPL(a, b)
    #define PROFILE_SCOPE(name) PerformanceProfiler::ScopedTimer PROFILE_CONCAT(__profiler_, __COUNTER__)(name)
    #define PROFILE_BEGIN(name) PerformanceProfiler::getInstance().beginSection(name)
    #define PROFILE_END(name) PerformanceProfiler::getInstance().endSection(name)
#else
    #define PROFILE_SCOPE(name) ((void)0)
    #define PROFILE_BEGIN(name) ((void)0)
    #define PROFILE_END(name) ((void)0)
#endif

class PerformanceProfiler
{
public:
    static constexpr int MAX_SECTIONS = 32;
    static constexpr int HISTORY_SIZE = 256;  // Keep last N measurements

    struct SectionStats
    {
        std::string name;
        std::atomic<double> totalTimeUs{0.0};
        std::atomic<double> minTimeUs{std::numeric_limits<double>::max()};
        std::atomic<double> maxTimeUs{0.0};
        std::atomic<uint64_t> callCount{0};
        std::atomic<double> avgTimeUs{0.0};

        // Rolling history for percentile calculations
        std::array<double, HISTORY_SIZE> history;
        std::atomic<int> historyIndex{0};

        void reset()
        {
            totalTimeUs.store(0.0);
            minTimeUs.store(std::numeric_limits<double>::max());
            maxTimeUs.store(0.0);
            callCount.store(0);
            avgTimeUs.store(0.0);
            historyIndex.store(0);
        }
    };

    static PerformanceProfiler& getInstance()
    {
        static PerformanceProfiler instance;
        return instance;
    }

    //==========================================================================
    // Manual profiling API

    void beginSection(const char* name)
    {
        int idx = getSectionIndex(name);
        if (idx >= 0)
        {
            sectionStartTimes[idx] = juce::Time::getHighResolutionTicks();
        }
    }

    void endSection(const char* name)
    {
        auto endTime = juce::Time::getHighResolutionTicks();
        int idx = getSectionIndex(name);
        if (idx >= 0 && sectionStartTimes[idx] > 0)
        {
            auto startTime = sectionStartTimes[idx];
            double elapsedUs = juce::Time::highResolutionTicksToSeconds(endTime - startTime) * 1000000.0;

            recordMeasurement(idx, elapsedUs);
            sectionStartTimes[idx] = 0;
        }
    }

    //==========================================================================
    // Statistics access (thread-safe, call from UI thread)

    const SectionStats* getStats(int index) const
    {
        if (index >= 0 && index < numSections.load())
            return &sections[index];
        return nullptr;
    }

    int getNumSections() const { return numSections.load(); }

    void resetAllStats()
    {
        for (int i = 0; i < numSections.load(); ++i)
            sections[i].reset();
    }

    //==========================================================================
    // Reporting

    juce::String getReport() const
    {
        juce::String report = "=== Performance Report ===\n\n";

        for (int i = 0; i < numSections.load(); ++i)
        {
            const auto& s = sections[i];
            if (s.callCount.load() > 0)
            {
                report += juce::String::formatted(
                    "%-30s  avg: %8.2f us  min: %8.2f us  max: %8.2f us  calls: %llu\n",
                    s.name.c_str(),
                    s.avgTimeUs.load(),
                    s.minTimeUs.load(),
                    s.maxTimeUs.load(),
                    s.callCount.load()
                );
            }
        }

        return report;
    }

    //==========================================================================
    // RAII scope timer

    class ScopedTimer
    {
    public:
        explicit ScopedTimer(const char* name) : sectionName(name)
        {
            PerformanceProfiler::getInstance().beginSection(name);
        }

        ~ScopedTimer()
        {
            PerformanceProfiler::getInstance().endSection(sectionName);
        }

    private:
        const char* sectionName;
    };

private:
    PerformanceProfiler() = default;

    std::array<SectionStats, MAX_SECTIONS> sections;
    std::array<juce::int64, MAX_SECTIONS> sectionStartTimes{};
    std::atomic<int> numSections{0};
    juce::SpinLock sectionLock;

    int getSectionIndex(const char* name)
    {
        // First, try to find existing section (fast path, no lock)
        for (int i = 0; i < numSections.load(); ++i)
        {
            if (sections[i].name == name)
                return i;
        }

        // Need to create new section (slow path, with lock)
        juce::SpinLock::ScopedLockType lock(sectionLock);

        // Double-check after acquiring lock
        for (int i = 0; i < numSections.load(); ++i)
        {
            if (sections[i].name == name)
                return i;
        }

        // Create new section
        int newIdx = numSections.load();
        if (newIdx < MAX_SECTIONS)
        {
            sections[newIdx].name = name;
            sections[newIdx].reset();
            numSections.store(newIdx + 1);
            return newIdx;
        }

        return -1;  // Too many sections
    }

    void recordMeasurement(int idx, double timeUs)
    {
        auto& s = sections[idx];

        // Update statistics atomically
        s.callCount.fetch_add(1);

        double oldTotal = s.totalTimeUs.load();
        s.totalTimeUs.store(oldTotal + timeUs);

        // Update min
        double oldMin = s.minTimeUs.load();
        while (timeUs < oldMin && !s.minTimeUs.compare_exchange_weak(oldMin, timeUs)) {}

        // Update max
        double oldMax = s.maxTimeUs.load();
        while (timeUs > oldMax && !s.maxTimeUs.compare_exchange_weak(oldMax, timeUs)) {}

        // Update average
        uint64_t count = s.callCount.load();
        s.avgTimeUs.store(s.totalTimeUs.load() / static_cast<double>(count));

        // Record in history (for percentiles)
        int histIdx = s.historyIndex.fetch_add(1) % HISTORY_SIZE;
        s.history[histIdx] = timeUs;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformanceProfiler)
};
