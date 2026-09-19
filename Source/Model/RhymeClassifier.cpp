#include "RhymeClassifier.h"
#include "SyllableSplitter.h"
#include <algorithm>
#include <cmath>

namespace CompassCadence
{

RhymeClassifier::RhymeClassifier()
{
}

const std::vector<juce::Colour>& RhymeClassifier::getHighlighterPalette()
{
    // 20 distinct, high-contrast pastel highlighter tones with realistic notebook ink opacity
    static const std::vector<juce::Colour> palette = {
        juce::Colour(0xBA7DD3FC), // 1. Sky Blue
        juce::Colour(0xBAF472B6), // 2. Bubblegum Rose
        juce::Colour(0xBAFBBF24), // 3. Amber Gold
        juce::Colour(0xBAA78BFA), // 4. Soft Lavender
        juce::Colour(0xBA34D399), // 5. Mint Green
        juce::Colour(0xBAFB923C), // 6. Coral Peach
        juce::Colour(0xBAFB7185), // 7. Pastel Crimson
        juce::Colour(0xBA22D3EE), // 8. Bright Cyan / Aqua
        juce::Colour(0xBAFDE047), // 9. Lemon Highlighter
        juce::Colour(0xBA818CF8), // 10. Periwinkle
        juce::Colour(0xBAA3E635), // 11. Lime Chartreuse
        juce::Colour(0xBAC084FC), // 12. Orchid Purple
        juce::Colour(0xBA6EE7B7), // 13. Seafoam
        juce::Colour(0xBAFDBA74), // 14. Warm Apricot
        juce::Colour(0xBA93C5FD), // 15. Ice Blue
        juce::Colour(0xBAFDA4AF), // 16. Blush Pink
        juce::Colour(0xBAFCD34D), // 17. Sun Yellow
        juce::Colour(0xBA5EEAD4), // 18. Caribbean Teal
        juce::Colour(0xBAE9D5FF), // 19. Pale Lilac
        juce::Colour(0xBAC4B5FD)  // 20. Dusk Mauve
    };
    return palette;
}

juce::String RhymeClassifier::extractRhymeKey(const juce::String& rawSyllable)
{
    juce::String s = rawSyllable.trim().toLowerCase();
    if (s.isEmpty())
        return {};

    // Non-terminal syllable ending in hyphen (e.g. "re-", "po-", "un-") is not a terminal rhyme
    if (s.endsWith("-"))
        return {};

    // If squished cell contains multiple words, rhyme the final word
    int lastSpace = s.lastIndexOfChar(' ');
    if (lastSpace >= 0)
        s = s.substring(lastSpace + 1).trim();

    // Strip punctuation
    juce::String clean;
    for (int i = 0; i < s.length(); ++i)
    {
        juce::juce_wchar c = s[i];
        if (juce::CharacterFunctions::isLetter(c))
            clean += c;
    }

    if (clean.isEmpty())
        return {};

    // 1. Curated CMUDict phonetic dictionary for common English words & rhymes
    static const std::unordered_map<std::string, const char*> cmuDict = {
        // [UW] phoneme (to, do, you, through, blue, new)
        { "to", "UW" }, { "too", "UW" }, { "two", "UW" }, { "do", "UW" }, { "you", "UW" },
        { "who", "UW" }, { "through", "UW" }, { "true", "UW" }, { "blue", "UW" }, { "clue", "UW" },
        { "sue", "UW" }, { "due", "UW" }, { "new", "UW" }, { "knew", "UW" }, { "flew", "UW" },
        { "crew", "UW" }, { "grew", "UW" }, { "threw", "UW" }, { "shoe", "UW" }, { "lupe", "UW_P" },

        // [OW] phoneme (so, no, go, flow, slow, know)
        { "so", "OW" }, { "no", "OW" }, { "go", "OW" }, { "pro", "OW" }, { "flow", "OW" },
        { "grow", "OW" }, { "slow", "OW" }, { "show", "OW" }, { "know", "OW" }, { "though", "OW" },
        { "glow", "OW" }, { "blow", "OW" }, { "throw", "OW" }, { "foe", "OW" }, { "toe", "OW" },

        // [EY] phoneme (day, way, say, play, they)
        { "day", "EY" }, { "way", "EY" }, { "say", "EY" }, { "may", "EY" }, { "play", "EY" },
        { "stay", "EY" }, { "away", "EY" }, { "pay", "EY" }, { "they", "EY" }, { "weigh", "EY" },
        { "lay", "EY" }, { "gray", "EY" }, { "hey", "EY" }, { "okay", "EY" },

        // [IY] phoneme (me, see, be, we, free)
        { "me", "IY" }, { "see", "IY" }, { "be", "IY" }, { "we", "IY" }, { "free", "IY" },
        { "tree", "IY" }, { "she", "IY" }, { "he", "IY" }, { "glee", "IY" }, { "key", "IY" },
        { "knee", "IY" }, { "sea", "IY" }, { "agree", "IY" },

        // [AY] phoneme (my, fly, high, by, die, try)
        { "my", "AY" }, { "fly", "AY" }, { "high", "AY" }, { "by", "AY" }, { "buy", "AY" },
        { "die", "AY" }, { "lie", "AY" }, { "tie", "AY" }, { "pie", "AY" }, { "try", "AY" },
        { "why", "AY" }, { "sky", "AY" }, { "cry", "AY" },

        // [AY_T] phoneme (night, right, light, white, might)
        { "night", "AY_T" }, { "right", "AY_T" }, { "might", "AY_T" }, { "light", "AY_T" },
        { "sight", "AY_T" }, { "bright", "AY_T" }, { "tight", "AY_T" }, { "fight", "AY_T" },
        { "white", "AY_T" }, { "bite", "AY_T" }, { "kite", "AY_T" }, { "quite", "AY_T" },

        // [AW_N] & [AW_ND] phonemes (down, town / sound, round, ground)
        { "down", "AW_N" }, { "town", "AW_N" }, { "brown", "AW_N" }, { "crown", "AW_N" },
        { "drown", "AW_N" }, { "clown", "AW_N" }, { "frown", "AW_N" }, { "noun", "AW_N" },
        { "sound", "AW_ND" }, { "round", "AW_ND" }, { "ground", "AW_ND" }, { "found", "AW_ND" },
        { "bound", "AW_ND" }, { "wound", "AW_ND" }, { "hound", "AW_ND" },

        // [AW_T] phoneme (out, shout, doubt)
        { "out", "AW_T" }, { "shout", "AW_T" }, { "doubt", "AW_T" }, { "about", "AW_T" },

        // [AO_L] phoneme (all, call, fall, ball, wall, tall)
        { "all", "AO_L" }, { "call", "AO_L" }, { "fall", "AO_L" }, { "ball", "AO_L" },
        { "wall", "AO_L" }, { "tall", "AO_L" }, { "small", "AO_L" }, { "hall", "AO_L" },

        // [EH_T] phoneme (set, let, get, wet, met, bet)
        { "set", "EH_T" }, { "let", "EH_T" }, { "get", "EH_T" }, { "wet", "EH_T" },
        { "met", "EH_T" }, { "bet", "EH_T" }, { "net", "EH_T" }, { "yet", "EH_T" },
        { "pet", "EH_T" }, { "debt", "EH_T" }, { "threat", "EH_T" }, { "forget", "EH_T" },

        // [UH_T] vs [AH_T] vs [IH_T]
        { "put", "UH_T" }, { "foot", "UH_T" },
        { "but", "AH_T" }, { "cut", "AH_T" }, { "shut", "AH_T" }, { "strut", "AH_T" },
        { "what", "AH_T" }, { "nut", "AH_T" }, { "gut", "AH_T" },
        { "it", "IH_T" }, { "sit", "IH_T" }, { "fit", "IH_T" }, { "hit", "IH_T" },
        { "bit", "IH_T" }, { "lit", "IH_T" }, { "quit", "IH_T" }, { "shit", "IH_T" },

        // [UH_K] phoneme (look, book, took)
        { "look", "UH_K" }, { "book", "UH_K" }, { "took", "UH_K" }, { "cook", "UH_K" },
        { "hook", "UH_K" }, { "shook", "UH_K" },

        // [EY_K] phoneme (make, take, break, shake, wake)
        { "make", "EY_K" }, { "take", "EY_K" }, { "break", "EY_K" }, { "shake", "EY_K" },
        { "wake", "EY_K" }, { "lake", "EY_K" }, { "fake", "EY_K" }, { "snake", "EY_K" },

        // [EY_S] phoneme (place, pace, face, space, race, case)
        { "place", "EY_S" }, { "pace", "EY_S" }, { "face", "EY_S" }, { "space", "EY_S" },
        { "race", "EY_S" }, { "case", "EY_S" }, { "base", "EY_S" }, { "grace", "EY_S" },

        // [AE_ND] phoneme (stand, hand, land, understand, and)
        { "stand", "AE_ND" }, { "hand", "AE_ND" }, { "land", "AE_ND" }, { "band", "AE_ND" },
        { "grand", "AE_ND" }, { "understand", "AE_ND" }, { "and", "AE_ND" },

        // [AE_N] phoneme (man, can, plan, fan)
        { "man", "AE_N" }, { "can", "AE_N" }, { "plan", "AE_N" }, { "fan", "AE_N" },
        { "pan", "AE_N" }, { "ran", "AE_N" },

        // [AY_N] phoneme (line, shine, mine, fine, sign, wine)
        { "line", "AY_N" }, { "shine", "AY_N" }, { "mine", "AY_N" }, { "fine", "AY_N" },
        { "sign", "AY_N" }, { "wine", "AY_N" }, { "nine", "AY_N" }, { "define", "AY_N" },

        // [AY_M] phoneme (time, rhyme, climb, prime)
        { "time", "AY_M" }, { "rhyme", "AY_M" }, { "climb", "AY_M" }, { "dime", "AY_M" },
        { "prime", "AY_M" }, { "crime", "AY_M" },

        // [IH_NG] phoneme (thing, sing, ring, bring, king)
        { "thing", "IH_NG" }, { "sing", "IH_NG" }, { "ring", "IH_NG" }, { "bring", "IH_NG" },
        { "king", "IH_NG" }, { "wing", "IH_NG" }, { "spring", "IH_NG" },

        // [EH_D] phoneme (head, bed, red, said, dead, instead)
        { "head", "EH_D" }, { "bed", "EH_D" }, { "red", "EH_D" }, { "said", "EH_D" },
        { "dead", "EH_D" }, { "lead", "EH_D" }, { "instead", "EH_D" }, { "ahead", "EH_D" },

        // [UH_D] phoneme (good, could, would, should, wood)
        { "good", "UH_D" }, { "could", "UH_D" }, { "would", "UH_D" }, { "should", "UH_D" },
        { "wood", "UH_D" }, { "hood", "UH_D" }, { "stood", "UH_D" },

        // [AO_R] phoneme (more, door, floor, four, pour, core, war)
        { "more", "AO_R" }, { "door", "AO_R" }, { "floor", "AO_R" }, { "four", "AO_R" },
        { "pour", "AO_R" }, { "core", "AO_R" }, { "store", "AO_R" }, { "war", "AO_R" },

        // [OW_N] phoneme (phone, bone, stone, alone, tone, zone, known)
        { "phone", "OW_N" }, { "bone", "OW_N" }, { "stone", "OW_N" }, { "alone", "OW_N" },
        { "tone", "OW_N" }, { "zone", "OW_N" }, { "known", "OW_N" }, { "grown", "OW_N" },

        // [OW_M] phoneme (home, roam, dome)
        { "home", "OW_M" }, { "roam", "OW_M" }, { "dome", "OW_M" }, { "foam", "OW_M" },

        // [OW_P] phoneme (hope, rope, cope, scope)
        { "hope", "OW_P" }, { "rope", "OW_P" }, { "cope", "OW_P" }, { "scope", "OW_P" },

        // [IY_V] phoneme (believe, achieve, receive, leave)
        { "lieve", "IY_V" }, { "believe", "IY_V" }, { "achieve", "IY_V" }, { "receive", "IY_V" },
        { "leave", "IY_V" },

        // [AH_N] phoneme (fun, sun, run, one, done, won, son)
        { "fun", "AH_N" }, { "sun", "AH_N" }, { "run", "AH_N" }, { "one", "AH_N" },
        { "done", "AH_N" }, { "won", "AH_N" }, { "son", "AH_N" }, { "gun", "AH_N" },

        // [AA_T] phoneme (got, not, hot, lot, shot, spot)
        { "got", "AA_T" }, { "not", "AA_T" }, { "hot", "AA_T" }, { "lot", "AA_T" },
        { "shot", "AA_T" }, { "spot", "AA_T" }, { "plot", "AA_T" },

        // [AE_P] phoneme (cap, map, tap, rap, trap)
        { "cap", "AE_P" }, { "map", "AE_P" }, { "tap", "AE_P" }, { "rap", "AE_P" },
        { "trap", "AE_P" }, { "clap", "AE_P" }, { "snap", "AE_P" },

        // [IH_NGK] phoneme (think, sink, drink, link, pink)
        { "think", "IH_NGK" }, { "sink", "IH_NGK" }, { "drink", "IH_NGK" }, { "link", "IH_NGK" },
        { "pink", "IH_NGK" }, { "blink", "IH_NGK" },

        // [AE_SH] phoneme (cash, crash, flash, smash)
        { "cash", "AE_SH" }, { "crash", "AE_SH" }, { "flash", "AE_SH" }, { "smash", "AE_SH" },

        // [AE_K] phoneme (back, black, track, crack)
        { "back", "AE_K" }, { "black", "AE_K" }, { "track", "AE_K" }, { "crack", "AE_K" }
    };

    auto dictIt = cmuDict.find(clean.toStdString());
    if (dictIt != cmuDict.end())
        return dictIt->second;

    // 2. High-priority phonetic suffix rules
    if (clean.endsWith("tion") || clean.endsWith("sion"))
    {
        if (clean.endsWith("ection") || clean.endsWith("iction"))
            return "EH_K_SH_AH_N";
        return "SH_AH_N";
    }

    if (clean.endsWith("ight") || clean.endsWith("ite"))
        return "AY_T";

    if (clean.endsWith("ound"))
        return "AW_ND";

    if (clean.endsWith("ance") || clean.endsWith("ence"))
        return "AE_NS";

    if (clean.endsWith("ing") && clean.length() > 3)
        return "IH_NG";

    if (clean.endsWith("goin") || clean.endsWith("sayin") || clean.endsWith("makin"))
        return "OW_IH_N";

    // Adverb / adjective unstressed "-ly" / "-ry" endings (e.g. sorry, every, simply, actively, properly)
    if (clean.endsWith("ly") || clean.endsWith("ry") || clean.endsWith("ty") || clean.endsWith("dy"))
    {
        if (clean.length() >= 4)
            return "IY_TAIL";
    }

    // Silent 'e' vowel groups (e.g. make -> EY_K, time -> AY_M, bone -> OW_N)
    int len = clean.length();
    if (len >= 3 && clean.endsWith("e") && !clean.endsWith("ee"))
    {
        char v = clean[len - 3];
        char c = clean[len - 2];
        if (SyllableSplitter::isVowel(v) && !SyllableSplitter::isVowel(c))
        {
            juce::String vPhoneme;
            if (v == 'a') vPhoneme = "EY";
            else if (v == 'i' || v == 'y') vPhoneme = "AY";
            else if (v == 'o') vPhoneme = "OW";
            else if (v == 'u') vPhoneme = "UW";
            else if (v == 'e') vPhoneme = "IY";

            if (vPhoneme.isNotEmpty())
            {
                juce::String coda = juce::String::charToString(c).toUpperCase();
                return vPhoneme + "_" + coda;
            }
        }
    }

    // Common phonetic endings table
    static const std::pair<const char*, const char*> suffixMap[] = {
        { "all", "AO_L" }, { "ell", "EH_L" }, { "ill", "IH_L" }, { "ull", "AH_L" },
        { "ack", "AE_K" }, { "eck", "EH_K" }, { "ick", "IH_K" }, { "ock", "AA_K" }, { "uck", "AH_K" },
        { "ash", "AE_SH" }, { "esh", "EH_SH" }, { "ish", "IH_SH" }, { "ush", "AH_SH" },
        { "ate", "EY_T" }, { "ait", "EY_T" }, { "eigh", "EY" }, { "ain", "EY_N" }, { "ane", "EY_N" },
        { "eep", "IY_P" }, { "eap", "IY_P" }, { "eet", "IY_T" }, { "eat", "IY_T" },
        { "ook", "UH_K" }, { "ood", "UH_D" }, { "oot", "UW_T" }, { "oom", "UW_M" },
        { "out", "AW_T" }, { "own", "AW_N" }, { "ound", "AW_ND" },
        { "and", "AE_ND" }, { "end", "EH_ND" },
        { "art", "AA_RT" }, { "ard", "AA_RD" }, { "ark", "AA_RK" },
        { "ore", "AO_R" }, { "oor", "AO_R" }, { "our", "AO_R" },
        { "ire", "AY_R" }, { "yre", "AY_R" },
        { "ee", "IY" }, { "ea", "IY" }, { "ay", "EY" }, { "ai", "EY" },
        { "ow", "OW" }, { "oa", "OW" }, { "oo", "UW" }, { "ue", "UW" },
        { "oy", "OY" }, { "oi", "OY" },
        { "op", "AA_P" }, { "ap", "AE_P" }, { "ip", "IH_P" }, { "up", "AH_P" },
        { "ot", "AA_T" }, { "at", "AE_T" }, { "it", "IH_T" }, { "ut", "AH_T" }, { "et", "EH_T" },
        { "an", "AE_N" }, { "en", "EH_N" }, { "in", "IH_N" }, { "on", "AA_N" }, { "un", "AH_N" },
        { "ad", "AE_D" }, { "ed", "EH_D" }, { "id", "IH_D" }, { "od", "AA_D" }, { "ud", "AH_D" }
    };

    for (const auto& r : suffixMap)
    {
        if (clean.endsWith(r.first))
            return r.second;
    }

    // 3. Robust phonetic fallback: find the final vowel cluster and terminal consonant coda
    int lastVowelIdx = -1;
    for (int i = len - 1; i >= 0; --i)
    {
        if (SyllableSplitter::isVowel(clean[i]))
        {
            lastVowelIdx = i;
            break;
        }
    }

    if (lastVowelIdx >= 0)
    {
        char v = clean[lastVowelIdx];
        juce::String coda = clean.substring(lastVowelIdx + 1).toUpperCase();
        juce::String vKey;
        if (v == 'a') vKey = "AE";
        else if (v == 'e') vKey = "EH";
        else if (v == 'i') vKey = "IH";
        else if (v == 'o') vKey = "AA";
        else if (v == 'u') vKey = "AH";
        else if (v == 'y') vKey = "IY";

        if (coda.isNotEmpty())
            return vKey + "_" + coda;
        return vKey;
    }

    return clean.toUpperCase();
}

void RhymeClassifier::updateRhymeMap(const std::vector<juce::String>& allSyllables)
{
    keyToColour.clear();
    if (!enabled)
        return;

    // 1. Count occurrences of each phonetic rhyme key
    std::unordered_map<std::string, int> freq;
    std::vector<std::string> orderedKeys;

    for (const auto& syl : allSyllables)
    {
        juce::String key = extractRhymeKey(syl);
        if (key.isNotEmpty())
        {
            std::string k = key.toStdString();
            if (freq[k] == 0)
                orderedKeys.push_back(k);
            freq[k]++;
        }
    }

    // 2. Filter to active rhyme families (at least 2 occurrences)
    std::vector<std::pair<std::string, int>> activeFamilies;
    for (const auto& k : orderedKeys)
    {
        if (freq[k] >= 2)
            activeFamilies.push_back({ k, freq[k] });
    }

    // 3. Sort by occurrence count descending: most prominent rhyme schemes receive the primary palette hues
    std::sort(activeFamilies.begin(), activeFamilies.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second)
            return a.second > b.second;
        return a.first < b.first;
    });

    // 4. Assign UNIQUE distinct colors: ZERO modulo collisions!
    const auto& palette = getHighlighterPalette();
    for (size_t i = 0; i < activeFamilies.size(); ++i)
    {
        if (i < palette.size())
        {
            keyToColour[activeFamilies[i].first] = palette[i];
        }
        else
        {
            // For documents with > 20 distinct rhyme groups, distribute evenly across the chromatic wheel using golden ratio
            float hue = std::fmod((float)i * 0.618033988749895f, 1.0f);
            keyToColour[activeFamilies[i].first] = juce::Colour::fromHSV(hue, 0.40f, 0.96f, 0.65f);
        }
    }
}

juce::Colour RhymeClassifier::getHighlightForSyllable(const juce::String& syllable) const
{
    if (!enabled)
        return juce::Colours::transparentBlack;

    juce::String key = extractRhymeKey(syllable);
    if (key.isEmpty())
        return juce::Colours::transparentBlack;

    auto it = keyToColour.find(key.toStdString());
    if (it != keyToColour.end())
        return it->second;

    return juce::Colours::transparentBlack;
}

} // namespace CompassCadence
