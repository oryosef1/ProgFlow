#include "TimeSignatureTrack.h"
#include <algorithm>

//==============================================================================
// TimeSignatureEvent

juce::var TimeSignatureEvent::toVar() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("barPosition", barPosition);
    obj->setProperty("numerator", numerator);
    obj->setProperty("denominator", denominator);
    return juce::var(obj);
}

TimeSignatureEvent TimeSignatureEvent::fromVar(const juce::var& var)
{
    TimeSignatureEvent event;

    if (var.hasProperty("barPosition"))
        event.barPosition = var["barPosition"];

    if (var.hasProperty("numerator"))
        event.numerator = static_cast<int>(var["numerator"]);

    if (var.hasProperty("denominator"))
        event.denominator = static_cast<int>(var["denominator"]);

    return event;
}

//==============================================================================
// TimeSignatureTrack

TimeSignatureTrack::TimeSignatureTrack()
{
    // Always have an initial time signature at bar 0
    TimeSignatureEvent initial;
    initial.barPosition = 0.0;
    initial.numerator = 4;
    initial.denominator = 4;
    events.push_back(initial);
}

void TimeSignatureTrack::addEvent(const TimeSignatureEvent& event)
{
    // Check if event exists at this position
    for (auto& e : events)
    {
        if (std::abs(e.barPosition - event.barPosition) < 0.001)
        {
            e = event;
            return;
        }
    }

    events.push_back(event);
    sortEvents();
}

void TimeSignatureTrack::removeEventAt(double barPosition)
{
    // Don't remove the initial event at bar 0
    if (barPosition < 0.001)
        return;

    events.erase(
        std::remove_if(events.begin(), events.end(),
            [barPosition](const TimeSignatureEvent& e) {
                return std::abs(e.barPosition - barPosition) < 0.001;
            }),
        events.end()
    );
}

void TimeSignatureTrack::clearEvents()
{
    auto initial = getInitialTimeSignature();
    events.clear();

    TimeSignatureEvent first;
    first.barPosition = 0.0;
    first.numerator = initial.numerator;
    first.denominator = initial.denominator;
    events.push_back(first);
}

TimeSignatureEvent TimeSignatureTrack::getTimeSignatureAtBar(double barPosition) const
{
    if (events.empty())
    {
        TimeSignatureEvent defaultSig;
        defaultSig.barPosition = 0.0;
        defaultSig.numerator = 4;
        defaultSig.denominator = 4;
        return defaultSig;
    }

    int index = findEventIndexAt(barPosition);

    if (index < 0)
        return events[0];

    return events[static_cast<size_t>(index)];
}

TimeSignatureEvent TimeSignatureTrack::getInitialTimeSignature() const
{
    if (events.empty())
    {
        TimeSignatureEvent defaultSig;
        defaultSig.barPosition = 0.0;
        defaultSig.numerator = 4;
        defaultSig.denominator = 4;
        return defaultSig;
    }
    return events[0];
}

void TimeSignatureTrack::setInitialTimeSignature(int numerator, int denominator)
{
    if (events.empty())
    {
        TimeSignatureEvent initial;
        initial.barPosition = 0.0;
        initial.numerator = numerator;
        initial.denominator = denominator;
        events.push_back(initial);
    }
    else
    {
        events[0].numerator = numerator;
        events[0].denominator = denominator;
    }
}

double TimeSignatureTrack::barsToBeats(double bars) const
{
    if (bars <= 0.0)
        return 0.0;

    double beats = 0.0;
    double currentBar = 0.0;

    for (size_t i = 0; i < events.size(); ++i)
    {
        const auto& event = events[i];
        double nextBar = (i + 1 < events.size()) ? events[i + 1].barPosition : bars;

        if (nextBar > bars)
            nextBar = bars;

        if (currentBar >= nextBar)
            continue;

        double barRange = nextBar - std::max(currentBar, event.barPosition);

        // Calculate beats in this range
        // Beats per bar = numerator * (4 / denominator)
        // For 4/4: 4 * 1 = 4 beats per bar
        // For 3/4: 3 * 1 = 3 beats per bar
        // For 6/8: 6 * 0.5 = 3 beats per bar (in quarter notes)
        double beatsPerBar = event.numerator * (4.0 / event.denominator);
        beats += barRange * beatsPerBar;

        currentBar = nextBar;

        if (currentBar >= bars)
            break;
    }

    return beats;
}

double TimeSignatureTrack::beatsToBar(double beats) const
{
    if (beats <= 0.0)
        return 0.0;

    double bars = 0.0;
    double currentBeats = 0.0;

    for (size_t i = 0; i < events.size(); ++i)
    {
        const auto& event = events[i];
        double beatsPerBar = event.numerator * (4.0 / event.denominator);

        // Calculate how many beats until next event
        double nextBarPosition = (i + 1 < events.size())
            ? events[i + 1].barPosition
            : 10000.0;

        double barsInThisSection = nextBarPosition - event.barPosition;
        double beatsInThisSection = barsInThisSection * beatsPerBar;

        if (currentBeats + beatsInThisSection >= beats)
        {
            // Target is within this section
            double remainingBeats = beats - currentBeats;
            bars += remainingBeats / beatsPerBar;
            return bars;
        }

        bars += barsInThisSection;
        currentBeats += beatsInThisSection;
    }

    return bars;
}

int TimeSignatureTrack::getBeatsInBar(double barPosition) const
{
    auto timeSig = getTimeSignatureAtBar(barPosition);
    return timeSig.numerator;
}

juce::var TimeSignatureTrack::toVar() const
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

void TimeSignatureTrack::fromVar(const juce::var& var)
{
    events.clear();

    if (var.hasProperty("events"))
    {
        auto* eventArray = var["events"].getArray();
        if (eventArray)
        {
            for (const auto& eventVar : *eventArray)
            {
                events.push_back(TimeSignatureEvent::fromVar(eventVar));
            }
        }
    }

    // Ensure we have at least an initial event
    if (events.empty())
    {
        TimeSignatureEvent initial;
        initial.barPosition = 0.0;
        initial.numerator = 4;
        initial.denominator = 4;
        events.push_back(initial);
    }

    sortEvents();
}

void TimeSignatureTrack::sortEvents()
{
    std::stable_sort(events.begin(), events.end(),
        [](const TimeSignatureEvent& a, const TimeSignatureEvent& b) {
            return a.barPosition < b.barPosition;
        });
}

int TimeSignatureTrack::findEventIndexAt(double barPosition) const
{
    int result = -1;

    for (size_t i = 0; i < events.size(); ++i)
    {
        if (events[i].barPosition <= barPosition)
            result = static_cast<int>(i);
        else
            break;
    }

    return result;
}
