#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <numeric>
#include <cmath>
#include <algorithm>

namespace CompassCadence
{

struct PlayheadLocation
{
    int bar = 0;
    int pulse = 0;
    int syllableInPulse = 0;
    int globalSyllableIndex = 0;
    double barProgress = 0.0;
};

class MetricNotation
{
public:
    MetricNotation();
    MetricNotation(int beats, const std::vector<int>& syllables);

    int getBeatsPerBar() const noexcept { return beatsPerBar; }
    void setBeatsPerBar(int beats);

    int getPulseCount() const noexcept { return (int)syllablesPerPulse.size(); }
    void setPulseCount(int count, int defaultSubdivisions = 4);

    const std::vector<int>& getSyllablesPerPulse() const noexcept { return syllablesPerPulse; }
    int getSyllablesForPulse(int pulseIndex) const;
    void setSyllablesForPulse(int pulseIndex, int count);

    int getTotalSyllables() const;
    int getGlobalSyllableIndex(int pulseIndex, int syllableInPulse) const;
    std::pair<int, int> getPulseAndSyllableFromGlobal(int globalIndex) const;

    PlayheadLocation calculateLocation(double totalBeats) const;
    PlayheadLocation calculateLocationInBar(int barIndex, double beatInBar, double bpb) const;

    juce::String toNotationString() const;
    static MetricNotation fromNotationString(const juce::String& text, int defaultBeats = 4);

    void addPulse(int syllables = 4);
    void removePulse(int pulseIndex);

    bool operator==(const MetricNotation& other) const;
    bool operator!=(const MetricNotation& other) const { return !(*this == other); }

private:
    int beatsPerBar = 4;
    std::vector<int> syllablesPerPulse { 4, 4, 4, 4 };
};

} // namespace CompassCadence
