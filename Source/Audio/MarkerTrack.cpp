#include "MarkerTrack.h"
#include <algorithm>

//==============================================================================
// Marker

juce::var Marker::toVar() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("id", id);
    obj->setProperty("name", name);
    obj->setProperty("beatPosition", beatPosition);
    obj->setProperty("colour", static_cast<juce::int64>(colour.getARGB()));
    return juce::var(obj);
}

Marker Marker::fromVar(const juce::var& var)
{
    Marker marker;

    if (var.hasProperty("id"))
        marker.id = var["id"].toString();

    if (var.hasProperty("name"))
        marker.name = var["name"].toString();

    if (var.hasProperty("beatPosition"))
        marker.beatPosition = var["beatPosition"];

    if (var.hasProperty("colour"))
        marker.colour = juce::Colour(static_cast<juce::uint32>(static_cast<juce::int64>(var["colour"])));

    return marker;
}

//==============================================================================
// MarkerTrack

Marker* MarkerTrack::addMarker(double beatPosition, const juce::String& name)
{
    Marker marker;
    marker.beatPosition = beatPosition;
    marker.name = name.isEmpty() ? generateMarkerName() : name;

    markers.push_back(marker);
    sortMarkers();

    // Return pointer to the added marker
    for (auto& m : markers)
    {
        if (m.id == marker.id)
            return &m;
    }

    return nullptr;
}

Marker* MarkerTrack::addMarker(const Marker& marker)
{
    markers.push_back(marker);
    sortMarkers();

    // Return pointer to the added marker
    for (auto& m : markers)
    {
        if (m.id == marker.id)
            return &m;
    }

    return nullptr;
}

void MarkerTrack::removeMarker(const juce::String& id)
{
    markers.erase(
        std::remove_if(markers.begin(), markers.end(),
            [&id](const Marker& m) {
                return m.id == id;
            }),
        markers.end()
    );
}

void MarkerTrack::removeMarkerAt(double beatPosition, double tolerance)
{
    markers.erase(
        std::remove_if(markers.begin(), markers.end(),
            [beatPosition, tolerance](const Marker& m) {
                return std::abs(m.beatPosition - beatPosition) <= tolerance;
            }),
        markers.end()
    );
}

void MarkerTrack::clearMarkers()
{
    markers.clear();
}

Marker* MarkerTrack::getMarker(const juce::String& id)
{
    for (auto& m : markers)
    {
        if (m.id == id)
            return &m;
    }
    return nullptr;
}

const Marker* MarkerTrack::getMarker(const juce::String& id) const
{
    for (const auto& m : markers)
    {
        if (m.id == id)
            return &m;
    }
    return nullptr;
}

const Marker* MarkerTrack::getNextMarker(double beatPosition) const
{
    for (const auto& m : markers)
    {
        if (m.beatPosition > beatPosition + 0.001)
            return &m;
    }
    return nullptr;
}

const Marker* MarkerTrack::getPreviousMarker(double beatPosition) const
{
    const Marker* result = nullptr;

    for (const auto& m : markers)
    {
        if (m.beatPosition < beatPosition - 0.001)
            result = &m;
        else
            break;
    }

    return result;
}

const Marker* MarkerTrack::getNearestMarker(double beatPosition) const
{
    if (markers.empty())
        return nullptr;

    const Marker* nearest = &markers[0];
    double nearestDist = std::abs(markers[0].beatPosition - beatPosition);

    for (const auto& m : markers)
    {
        double dist = std::abs(m.beatPosition - beatPosition);
        if (dist < nearestDist)
        {
            nearestDist = dist;
            nearest = &m;
        }
    }

    return nearest;
}

Marker* MarkerTrack::getMarkerAt(double beatPosition, double tolerance)
{
    for (auto& m : markers)
    {
        if (std::abs(m.beatPosition - beatPosition) <= tolerance)
            return &m;
    }
    return nullptr;
}

void MarkerTrack::renameMarker(const juce::String& id, const juce::String& newName)
{
    if (auto* marker = getMarker(id))
        marker->name = newName;
}

void MarkerTrack::moveMarker(const juce::String& id, double newBeatPosition)
{
    if (auto* marker = getMarker(id))
    {
        marker->beatPosition = newBeatPosition;
        sortMarkers();
    }
}

void MarkerTrack::setMarkerColour(const juce::String& id, juce::Colour colour)
{
    if (auto* marker = getMarker(id))
        marker->colour = colour;
}

juce::var MarkerTrack::toVar() const
{
    juce::Array<juce::var> markerArray;

    for (const auto& marker : markers)
    {
        markerArray.add(marker.toVar());
    }

    auto* obj = new juce::DynamicObject();
    obj->setProperty("markers", markerArray);
    return juce::var(obj);
}

void MarkerTrack::fromVar(const juce::var& var)
{
    markers.clear();

    if (var.hasProperty("markers"))
    {
        auto* markerArray = var["markers"].getArray();
        if (markerArray)
        {
            for (const auto& markerVar : *markerArray)
            {
                markers.push_back(Marker::fromVar(markerVar));
            }
        }
    }

    sortMarkers();
}

void MarkerTrack::sortMarkers()
{
    std::stable_sort(markers.begin(), markers.end(),
        [](const Marker& a, const Marker& b) {
            return a.beatPosition < b.beatPosition;
        });
}

juce::String MarkerTrack::generateMarkerName() const
{
    int index = static_cast<int>(markers.size()) + 1;
    return "Marker " + juce::String(index);
}
