#include "MetricNotation.h"

namespace CompassCadence
{

MetricNotation::MetricNotation()
    : beatsPerBar(4), syllablesPerPulse({ 4, 4, 4, 4 })
{
}

MetricNotation::MetricNotation(int beats, const std::vector<int>& syllables)
    : beatsPerBar(std::max(1, beats)), syllablesPerPulse(syllables)
{
    if (syllablesPerPulse.empty())
        syllablesPerPulse = { 4, 4, 4, 4 };
    for (auto& s : syllablesPerPulse)
        s = std::max(1, s);
}

void MetricNotation::setBeatsPerBar(int beats)
{
    beatsPerBar = std::max(1, beats);
}

void MetricNotation::setPulseCount(int count, int defaultSubdivisions)
{
    count = std::clamp(count, 1, 32);
    defaultSubdivisions = std::clamp(defaultSubdivisions, 1, 32);

    if ((int)syllablesPerPulse.size() < count)
    {
        while ((int)syllablesPerPulse.size() < count)
            syllablesPerPulse.push_back(defaultSubdivisions);
    }
    else if ((int)syllablesPerPulse.size() > count)
    {
        syllablesPerPulse.resize(count);
    }
}

int MetricNotation::getSyllablesForPulse(int pulseIndex) const
{
    if (pulseIndex >= 0 && pulseIndex < (int)syllablesPerPulse.size())
        return syllablesPerPulse[pulseIndex];
    return 1;
}

void MetricNotation::setSyllablesForPulse(int pulseIndex, int count)
{
    if (pulseIndex >= 0 && pulseIndex < (int)syllablesPerPulse.size())
    {
        syllablesPerPulse[pulseIndex] = std::clamp(count, 1, 32);
    }
}

int MetricNotation::getTotalSyllables() const
{
    int sum = 0;
    for (int s : syllablesPerPulse)
        sum += s;
    return sum;
}

int MetricNotation::getGlobalSyllableIndex(int pulseIndex, int syllableInPulse) const
{
    int index = 0;
    for (int p = 0; p < pulseIndex && p < (int)syllablesPerPulse.size(); ++p)
        index += syllablesPerPulse[p];
    return index + syllableInPulse;
}

std::pair<int, int> MetricNotation::getPulseAndSyllableFromGlobal(int globalIndex) const
{
    if (globalIndex < 0)
        return { 0, 0 };

    int remaining = globalIndex;
    for (int p = 0; p < (int)syllablesPerPulse.size(); ++p)
    {
        int count = syllablesPerPulse[p];
        if (remaining < count)
            return { p, remaining };
        remaining -= count;
    }

    if (!syllablesPerPulse.empty())
    {
        int lastPulse = (int)syllablesPerPulse.size() - 1;
        return { lastPulse, syllablesPerPulse[lastPulse] - 1 };
    }
    return { 0, 0 };
}

PlayheadLocation MetricNotation::calculateLocationInBar(int barIndex, double beatInBar, double bpb) const
{
    PlayheadLocation loc;
    loc.bar = barIndex;
    if (bpb <= 0.0) bpb = (double)beatsPerBar;
    if (beatInBar < 0.0) beatInBar = 0.0;
    if (beatInBar >= bpb) beatInBar = std::max(0.0, bpb - 1e-6);

    loc.barProgress = (bpb > 0.0) ? std::clamp(beatInBar / bpb, 0.0, 1.0) : 0.0;

    const int numPulses = getPulseCount();
    if (numPulses <= 0)
        return loc;

    double pulsePos = (beatInBar / bpb) * (double)numPulses;
    loc.pulse = std::clamp((int)std::floor(pulsePos), 0, numPulses - 1);

    double pulseFraction = pulsePos - (double)loc.pulse;
    int countInPulse = getSyllablesForPulse(loc.pulse);
    loc.syllableInPulse = std::clamp((int)std::floor(pulseFraction * (double)countInPulse), 0, countInPulse - 1);

    loc.globalSyllableIndex = getGlobalSyllableIndex(loc.pulse, loc.syllableInPulse);
    return loc;
}

PlayheadLocation MetricNotation::calculateLocation(double totalBeats) const
{
    if (totalBeats < 0.0)
        totalBeats = 0.0;

    const double bpb = (double)beatsPerBar;
    int bar = (int)std::floor(totalBeats / bpb);
    double beatInBar = totalBeats - ((double)bar * bpb);
    return calculateLocationInBar(bar, beatInBar, bpb);
}

juce::String MetricNotation::toNotationString() const
{
    juce::String s = "[";
    bool multiDigit = false;
    for (int count : syllablesPerPulse)
    {
        if (count >= 10)
        {
            multiDigit = true;
            break;
        }
    }

    for (size_t i = 0; i < syllablesPerPulse.size(); ++i)
    {
        if (i > 0 && multiDigit)
            s += ",";
        s += juce::String(syllablesPerPulse[i]);
    }
    s += "]/" + juce::String(getPulseCount()) + ":" + juce::String(beatsPerBar);
    return s;
}

MetricNotation MetricNotation::fromNotationString(const juce::String& rawText, int defaultBeats)
{
    juce::String text = rawText.trim();
    if (text.isEmpty())
        return MetricNotation(defaultBeats, { 4, 4, 4, 4 });

    int beats = defaultBeats;
    int specifiedPulses = -1;

    // Check for "/" ratio part, e.g. "/6:4" or "/3:4" or "/4"
    int slashIdx = text.indexOfChar('/');
    juce::String bracketPart = text;
    if (slashIdx >= 0)
    {
        bracketPart = text.substring(0, slashIdx).trim();
        juce::String ratioPart = text.substring(slashIdx + 1).trim();

        int colonIdx = ratioPart.indexOfChar(':');
        if (colonIdx >= 0)
        {
            specifiedPulses = ratioPart.substring(0, colonIdx).getIntValue();
            beats = ratioPart.substring(colonIdx + 1).getIntValue();
        }
        else
        {
            beats = ratioPart.getIntValue();
        }
    }

    if (beats <= 0)
        beats = defaultBeats > 0 ? defaultBeats : 4;

    // Parse bracket content
    int openBracket = bracketPart.indexOfChar('[');
    int closeBracket = bracketPart.lastIndexOfChar(']');
    juce::String inner = bracketPart;
    if (openBracket >= 0 && closeBracket > openBracket)
    {
        inner = bracketPart.substring(openBracket + 1, closeBracket).trim();
    }

    std::vector<int> parsedSyllables;

    // Check if delimited by commas or dashes, e.g. "3,3,3,2,2,2" or "3-3-3-2-2-2"
    if (inner.containsChar(',') || inner.containsChar('-') || inner.containsChar(' '))
    {
        juce::StringArray tokens;
        tokens.addTokens(inner, ",- ", "");
        for (const auto& tok : tokens)
        {
            int val = tok.trim().getIntValue();
            if (val > 0)
                parsedSyllables.push_back(val);
        }
    }
    else
    {
        // Continuous digits: e.g. "333222" or "4444" or "323"
        for (int i = 0; i < inner.length(); ++i)
        {
            juce::juce_wchar ch = inner[i];
            if (ch >= '1' && ch <= '9')
            {
                parsedSyllables.push_back(ch - '0');
            }
        }
    }

    if (parsedSyllables.empty())
    {
        if (specifiedPulses > 0)
        {
            parsedSyllables.assign(specifiedPulses, 4);
        }
        else
        {
            parsedSyllables = { 4, 4, 4, 4 };
        }
    }

    // If specified pulses given, adjust size
    if (specifiedPulses > 0 && specifiedPulses != (int)parsedSyllables.size())
    {
        if (specifiedPulses > (int)parsedSyllables.size())
        {
            int lastVal = parsedSyllables.back();
            while ((int)parsedSyllables.size() < specifiedPulses)
                parsedSyllables.push_back(lastVal);
        }
        else
        {
            parsedSyllables.resize(specifiedPulses);
        }
    }

    return MetricNotation(beats, parsedSyllables);
}

void MetricNotation::addPulse(int syllables)
{
    syllablesPerPulse.push_back(std::clamp(syllables, 1, 32));
}

void MetricNotation::removePulse(int pulseIndex)
{
    if (syllablesPerPulse.size() > 1 && pulseIndex >= 0 && pulseIndex < (int)syllablesPerPulse.size())
    {
        syllablesPerPulse.erase(syllablesPerPulse.begin() + pulseIndex);
    }
}

bool MetricNotation::operator==(const MetricNotation& other) const
{
    return beatsPerBar == other.beatsPerBar && syllablesPerPulse == other.syllablesPerPulse;
}

} // namespace CompassCadence
