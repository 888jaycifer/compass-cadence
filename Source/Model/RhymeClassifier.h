#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <unordered_map>
#include <vector>
#include <string>

namespace CompassCadence
{

class RhymeClassifier
{
public:
    RhymeClassifier();

    // Extracts the terminal phonetic rhyme sound key from a syllable/word
    static juce::String extractRhymeKey(const juce::String& rawSyllable);

    // Scans a collection of syllables and computes color assignments for recurring rhymes
    void updateRhymeMap(const std::vector<juce::String>& allSyllables);

    // Returns the highlighter color for a syllable, or transparent if no rhyme
    juce::Colour getHighlightForSyllable(const juce::String& syllable) const;

    // Checks if rhyme highlighting is active
    bool isEnabled() const noexcept { return enabled; }
    void setEnabled(bool e) noexcept { enabled = e; }

    static const std::vector<juce::Colour>& getHighlighterPalette();

private:
    bool enabled = true;
    std::unordered_map<std::string, juce::Colour> keyToColour;
};

} // namespace CompassCadence
