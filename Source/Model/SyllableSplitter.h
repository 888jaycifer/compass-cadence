#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace CompassCadence
{

class SyllableSplitter
{
public:
    static bool isVowel(juce::juce_wchar c)
    {
        c = juce::CharacterFunctions::toLowerCase(c);
        return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y';
    }

    // Splits a word into syllable fragments if it has multiple syllables
    static std::vector<juce::String> splitWordIntoSyllables(const juce::String& rawWord)
    {
        juce::String word = rawWord.trim();
        if (word.isEmpty())
            return {};

        // If the word already contains explicit hyphens or slashes, split by them
        if (word.containsChar('-') || word.containsChar('/'))
        {
            std::vector<juce::String> result;
            juce::StringArray parts;
            parts.addTokens(word, "-/", "");
            for (int i = 0; i < parts.size(); ++i)
            {
                juce::String clean = parts[i].trim();
                if (!clean.isEmpty())
                {
                    if (i < parts.size() - 1 && !clean.endsWith("-"))
                        clean += "-";
                    result.push_back(clean);
                }
            }
            if (!result.empty())
                return result;
        }

        // Separate leading and trailing punctuation
        int start = 0;
        while (start < word.length() && !juce::CharacterFunctions::isLetterOrDigit(word[start]))
            start++;

        int end = word.length() - 1;
        while (end >= start && !juce::CharacterFunctions::isLetterOrDigit(word[end]))
            end--;

        if (start > end)
            return { word };

        juce::String prefix = word.substring(0, start);
        juce::String core = word.substring(start, end + 1);
        juce::String suffix = word.substring(end + 1);

        juce::String lower = core.toLowerCase();
        int len = lower.length();

        // Short words (<= 3 chars) are almost always single syllable
        if (len <= 3)
            return { word };

        // Identify vowel group boundaries
        std::vector<int> vowelIndices;
        bool inVowel = false;
        for (int i = 0; i < len; ++i)
        {
            juce::juce_wchar c = lower[i];
            bool v = isVowel(c);

            // Handle silent 'e' at the end of words or before 's' / 'd'
            if (c == 'e' && !vowelIndices.empty())
            {
                // Case 1: Word ends directly with 'e' (e.g. "rhyme", "line", "make")
                if (i == len - 1)
                {
                    // check if preceding is 'l' with consonant before (e.g. -ble, -tle)
                    if (i >= 2 && lower[i - 1] == 'l' && !isVowel(lower[i - 2]))
                    {
                        vowelIndices.push_back(i - 1);
                    }
                    break;
                }

                // Case 2: Word ends with 'es' (e.g. "rhymes", "lines", "takes", "notes")
                if (i == len - 2 && lower[len - 1] == 's')
                {
                    juce::juce_wchar prev = (i >= 1) ? lower[i - 1] : 0;
                    bool isSibilant = (prev == 's' || prev == 'z' || prev == 'x' || prev == 'c' || prev == 'g' || prev == 'j');
                    if (i >= 2)
                    {
                        juce::String prev2 = lower.substring(i - 2, i);
                        if (prev2 == "ch" || prev2 == "sh")
                            isSibilant = true;
                    }

                    // Syllabic -les (e.g. "bottles", "tables", "candles")
                    if (i >= 2 && prev == 'l' && !isVowel(lower[i - 2]))
                    {
                        vowelIndices.push_back(i - 1);
                        break;
                    }

                    // If NOT preceded by a sibilant sound, the 'e' before 's' is completely silent!
                    if (!isSibilant)
                    {
                        break;
                    }
                }

                // Case 3: Word ends with 'ed' (e.g. "rhymed", "baked", "named", "grooved")
                if (i == len - 2 && lower[len - 1] == 'd')
                {
                    juce::juce_wchar prev = (i >= 1) ? lower[i - 1] : 0;
                    // Only -ted and -ded form an extra syllable in English
                    if (prev != 't' && prev != 'd')
                    {
                        break; // 'e' is silent
                    }
                }
            }

            if (v && !inVowel)
            {
                vowelIndices.push_back(i);
                inVowel = true;
            }
            else if (!v)
            {
                inVowel = false;
            }
        }

        // Handle syllabic consonant endings like -thm (rhythm), -sm (prism, chasm)
        if (lower.endsWith("thm") || lower.endsWith("sm"))
        {
            if (vowelIndices.empty() || vowelIndices.back() < len - 2)
                vowelIndices.push_back(len - 1);
        }

        // If 1 or 0 vowel groups, it's a single syllable
        if (vowelIndices.size() <= 1)
            return { word };

        // Determine split cut points between vowel groups
        std::vector<int> cutPoints;
        for (size_t k = 0; k + 1 < vowelIndices.size(); ++k)
        {
            int v1 = vowelIndices[k];
            int v2 = vowelIndices[k + 1];

            // Scan consonants between v1 and v2
            int consStart = v1 + 1;
            while (consStart < v2 && isVowel(lower[consStart]))
                consStart++;

            int consCount = v2 - consStart;
            int cut = consStart;

            if (consCount == 0)
            {
                // Adjacent vowel cluster that wasn't combined (e.g. "po-et", "di-al")
                cut = consStart;
            }
            else if (consCount == 1)
            {
                // V-C-V: typically split before the consonant (e.g. "o-pen", "mu-sic")
                cut = consStart;
            }
            else if (consCount >= 2)
            {
                // V-CC-V: split between consonants (e.g. "rap-per", "doc-tor")
                // Keep digraphs like 'th', 'sh', 'ch', 'ph' together if possible
                juce::String pair = lower.substring(consStart, consStart + 2);
                if (pair == "th" || pair == "sh" || pair == "ch" || pair == "ph" || pair == "wh" || pair == "ng" || pair == "ck")
                {
                    cut = consStart;
                }
                else
                {
                    cut = consStart + 1;
                }
            }

            if (cut > 0 && cut < len)
                cutPoints.push_back(cut);
        }

        if (cutPoints.empty())
            return { word };

        std::vector<juce::String> syllables;
        int lastCut = 0;
        for (size_t i = 0; i < cutPoints.size(); ++i)
        {
            int cut = cutPoints[i];
            juce::String syl = core.substring(lastCut, cut);
            if (i == 0 && !prefix.isEmpty())
                syl = prefix + syl;
            if (!syl.endsWith("-"))
                syl += "-";
            syllables.push_back(syl);
            lastCut = cut;
        }

        juce::String lastSyl = core.substring(lastCut);
        if (!suffix.isEmpty())
            lastSyl += suffix;
        syllables.push_back(lastSyl);

        return syllables;
    }

    // Splits an entire line or phrase into syllables
    static std::vector<juce::String> splitLineIntoSyllables(const juce::String& lineText)
    {
        std::vector<juce::String> allSyllables;
        juce::StringArray words;
        words.addTokens(lineText, " \t\r\n", "");

        for (const auto& w : words)
        {
            if (w.trim().isEmpty())
                continue;

            auto syls = splitWordIntoSyllables(w);
            for (auto& s : syls)
                allSyllables.push_back(s);
        }

        return allSyllables;
    }
};

} // namespace CompassCadence
