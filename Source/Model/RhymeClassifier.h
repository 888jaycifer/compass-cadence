#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <unordered_map>
#include <vector>
#include <string>

namespace CompassCadence
{

struct VowelSoundInfo
{
    juce::String key;          // "EY", "AE", etc.
    juce::String label;        // "long Ay", "short Uh", etc.
    juce::String examples;     // "day, cake, say"
    juce::Colour defaultColour;
};

struct DocumentSyllable
{
    int bar = 0;
    int sylIndex = 0;
    juce::String text;
};

class RhymeClassifier
{
public:
    enum class ColorMode
    {
        Off = 0,
        Rhymes = 1,
        Repeats = 2
    };

    RhymeClassifier();

    // Color mode controls (Off, Phonetic Rhymes, Repetition Sequence Detector)
    ColorMode getColorMode() const noexcept { return colorMode; }
    void setColorMode(ColorMode mode) noexcept { colorMode = mode; }
    void cycleColorMode() noexcept;

    // Backward compatibility for boolean checks
    bool isEnabled() const noexcept { return colorMode != ColorMode::Off; }
    void setEnabled(bool e) noexcept { colorMode = e ? ColorMode::Rhymes : ColorMode::Off; }

    // 15 Standard English Vowel Sound Catalog
    static const std::vector<VowelSoundInfo>& getVowelSoundCatalog();
    static const VowelSoundInfo* findVowelSound(const juce::String& key);

    // Extracts phonetic vowel sound key from a syllable/word (ignoring consonants)
    static juce::String extractRhymeKey(const juce::String& rawSyllable);

    // Context-aware vowel sound extractor (resolves surrounding syllables like "understandable")
    static juce::String extractRhymeKeyWithContext(const juce::String& rawSyllable,
                                                   const juce::String& prevSyllable = {},
                                                   const juce::String& nextSyllable = {});

    // Scans a collection of syllables and computes color assignments for recurring rhymes
    void updateRhymeMap(const std::vector<juce::String>& allSyllables);

    // Context-aware batch update for rhyme mode
    void updateRhymeMapWithContext(const std::vector<std::vector<juce::String>>& lineSyllables);

    // Repetition detector: scans document syllable stream for exact matching sequences of length >= 2
    void updateRepetitionMap(const std::vector<DocumentSyllable>& allSyllables);

    // Cell highlight resolution based on active colorMode
    juce::Colour getHighlightForCell(int barIndex, int globalSylIndex,
                                     const juce::String& currentText,
                                     const juce::String& prevSyl = {},
                                     const juce::String& nextSyl = {}) const;

    // Returns the highlighter color for a syllable, or transparent if no rhyme
    juce::Colour getHighlightForSyllable(const juce::String& syllable) const;
    juce::Colour getHighlightWithContext(const juce::String& syllable,
                                        const juce::String& prev = {},
                                        const juce::String& next = {}) const;

    // Vowel sound color palette customization
    juce::Colour getVowelColour(const juce::String& vowelKey) const;
    void setVowelColour(const juce::String& vowelKey, const juce::Colour& colour);
    void resetVowelColoursToDefaults();
    const std::unordered_map<std::string, juce::Colour>& getCustomVowelColours() const noexcept { return customColours; }
    void setCustomVowelColours(const std::unordered_map<std::string, juce::Colour>& map);

    static const std::vector<juce::Colour>& getHighlighterPalette();
    static juce::String cleanSyllableText(const juce::String& raw);

private:
    static uint64_t getCellKey(int bar, int syl) noexcept
    {
        return ((uint64_t)(uint32_t)bar << 32) | (uint32_t)syl;
    }

    ColorMode colorMode = ColorMode::Rhymes;
    std::unordered_map<std::string, juce::Colour> customColours;
    std::unordered_map<std::string, juce::Colour> keyToColour;
    std::unordered_map<uint64_t, juce::Colour> repeatCellColours;
};

} // namespace CompassCadence
