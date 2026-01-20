#include "TempoTrack.h"
#include <algorithm>

//==============================================================================
// TempoEvent

juce::var TempoEvent::toVar() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("beatPosition", beatPosition);
    obj->setProperty("bpm", bpm);
    obj->setProperty("rampType", static_cast<int>(rampType));
    return juce::var(obj);
}

TempoEvent TempoEvent::fromVar(const juce::var& var)
{
    TempoEvent event;

    if (var.hasProperty("beatPosition"))
        event.beatPosition = var["beatPosition"];

    if (var.hasProperty("bpm"))
        event.bpm = var["bpm"];

    if (var.hasProperty("rampType"))
        event.rampType = static_cast<TempoRampType>(static_cast<int>(var["rampType"]));

    return event;
}

//==============================================================================
// TempoTrack

TempoTrack::TempoTrack()
{
    // Always have an initial tempo event at beat 0
    TempoEvent initial;
    initial.beatPosition = 0.0;
    initial.bpm = 120.0;
    initial.rampType = TempoRampType::Instant;
    events.push_back(initial);
}

void TempoTrack::addEvent(const TempoEvent& event)
{
    // Check if event exists at this position
    for (auto& e : events)
    {
        if (std::abs(e.beatPosition - event.beatPosition) < 0.001)
        {
            e = event;
            return;
        }
    }

    events.push_back(event);
    sortEvents();
}

void TempoTrack::removeEventAt(double beatPosition)
{
    // Don't remove the initial event at beat 0
    if (beatPosition < 0.001)
        return;

    events.erase(
        std::remove_if(events.begin(), events.end(),
            [beatPosition](const TempoEvent& e) {
                return std::abs(e.beatPosition - beatPosition) < 0.001;
            }),
        events.end()
    );
}

void TempoTrack::clearEvents()
{
    double initialBpm = getInitialTempo();
    events.clear();

    TempoEvent initial;
    initial.beatPosition = 0.0;
    initial.bpm = initialBpm;
    initial.rampType = TempoRampType::Instant;
    events.push_back(initial);
}

double TempoTrack::getTempoAtBeat(double beatPosition) const
{
    if (events.empty())
        return 120.0;

    if (beatPosition <= 0.0)
        return events[0].bpm;

    int index = findEventIndexAt(beatPosition);

    if (index < 0)
        return events[0].bpm;

    const auto& currentEvent = events[static_cast<size_t>(index)];

    // Check if there's a next event with linear ramp
    if (static_cast<size_t>(index + 1) < events.size())
    {
        const auto& nextEvent = events[static_cast<size_t>(index + 1)];

        if (currentEvent.rampType == TempoRampType::Linear)
        {
            // Linear interpolation between events
            double t = (beatPosition - currentEvent.beatPosition) /
                       (nextEvent.beatPosition - currentEvent.beatPosition);
            return currentEvent.bpm + t * (nextEvent.bpm - currentEvent.bpm);
        }
    }

    return currentEvent.bpm;
}

double TempoTrack::getInitialTempo() const
{
    if (events.empty())
        return 120.0;
    return events[0].bpm;
}

void TempoTrack::setInitialTempo(double bpm)
{
    if (events.empty())
    {
        TempoEvent initial;
        initial.beatPosition = 0.0;
        initial.bpm = bpm;
        initial.rampType = TempoRampType::Instant;
        events.push_back(initial);
    }
    else
    {
        events[0].bpm = bpm;
    }
}

double TempoTrack::beatsToSeconds(double beats) const
{
    if (beats <= 0.0)
        return 0.0;

    double seconds = 0.0;
    double currentBeat = 0.0;

    for (size_t i = 0; i < events.size(); ++i)
    {
        const auto& event = events[i];
        double nextBeat = (i + 1 < events.size()) ? events[i + 1].beatPosition : beats;

        if (nextBeat > beats)
            nextBeat = beats;

        if (currentBeat >= nextBeat)
            continue;

        double beatRange = nextBeat - std::max(currentBeat, event.beatPosition);

        if (event.rampType == TempoRampType::Linear && i + 1 < events.size())
        {
            // For linear ramp, integrate over the tempo change
            // Using average tempo as approximation
            double startTempo = event.bpm;
            double endTempo = events[i + 1].bpm;
            double avgTempo = (startTempo + endTempo) / 2.0;
            seconds += (beatRange / avgTempo) * 60.0;
        }
        else
        {
            // Instant tempo - simple calculation
            seconds += (beatRange / event.bpm) * 60.0;
        }

        currentBeat = nextBeat;

        if (currentBeat >= beats)
            break;
    }

    return seconds;
}

double TempoTrack::secondsToBeats(double seconds) const
{
    if (seconds <= 0.0)
        return 0.0;

    double beats = 0.0;
    double currentSeconds = 0.0;

    for (size_t i = 0; i < events.size(); ++i)
    {
        const auto& event = events[i];
        double eventBpm = event.bpm;

        // Calculate how long until next event (in seconds)
        double eventDurationBeats = (i + 1 < events.size())
            ? events[i + 1].beatPosition - event.beatPosition
            : 10000.0;  // Large number for last event

        double eventDurationSeconds = (eventDurationBeats / eventBpm) * 60.0;

        if (currentSeconds + eventDurationSeconds >= seconds)
        {
            // Target is within this event's range
            double remainingSeconds = seconds - currentSeconds;
            beats += (remainingSeconds * eventBpm) / 60.0;
            return beats;
        }

        beats += eventDurationBeats;
        currentSeconds += eventDurationSeconds;
    }

    return beats;
}

double TempoTrack::getBeatRangeDuration(double startBeat, double endBeat) const
{
    return beatsToSeconds(endBeat) - beatsToSeconds(startBeat);
}

juce::var TempoTrack::toVar() const
{
    juce::Array<juce::var> eventArray;

    for (const auto& event : events)
    {
        eventArray.add(event.toVar());
    }

    auto* obj = new juce::DynamicObject();
    obj->setProperty("events", eventArray);
    return juce::var(obj);
}

void TempoTrack::fromVar(const juce::var& var)
{
    events.clear();

    if (var.hasProperty("events"))
    {
        auto* eventArray = var["events"].getArray();
        if (eventArray)
        {
            for (const auto& eventVar : *eventArray)
            {
                events.push_back(TempoEvent::fromVar(eventVar));
            }
        }
    }

    // Ensure we have at least an initial event
    if (events.empty())
    {
        TempoEvent initial;
        initial.beatPosition = 0.0;
        initial.bpm = 120.0;
        initial.rampType = TempoRampType::Instant;
        events.push_back(initial);
    }

    sortEvents();
}

void TempoTrack::sortEvents()
{
    std::stable_sort(events.begin(), events.end(),
        [](const TempoEvent& a, const TempoEvent& b) {
            return a.beatPosition < b.beatPosition;
        });
}

int TempoTrack::findEventIndexAt(double beatPosition) const
{
    int result = -1;

    for (size_t i = 0; i < events.size(); ++i)
    {
        if (events[i].beatPosition <= beatPosition)
            result = static_cast<int>(i);
        else
            break;
    }

    return result;
}
