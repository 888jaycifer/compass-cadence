#include "RhymeClassifier.h"
#include "SyllableSplitter.h"
#include <algorithm>
#include <cmath>

namespace CompassCadence
{

RhymeClassifier::RhymeClassifier()
{
}

const std::vector<VowelSoundInfo>& RhymeClassifier::getVowelSoundCatalog()
{
    static const std::vector<VowelSoundInfo> catalog = {
        { "EY",   "long Ay",   "day, cake, say, plain, isolated 'a'",   juce::Colour(0xBAFDBA74) }, // 1. Warm Peach/Apricot
        { "AE",   "short Ah",  "cat, stand, back, rap, track",          juce::Colour(0xBAFDE047) }, // 2. Lemon Yellow
        { "IY",   "long Ee",   "see, beat, deep, me, feel",             juce::Colour(0xBA7DD3FC) }, // 3. Sky Blue
        { "EH",   "short Eh",  "bed, set, check, red, let",             juce::Colour(0xBA86EFAC) }, // 4. Mint Green
        { "AY",   "long Eye",  "my, shine, time, night, right, fly",    juce::Colour(0xBAC4B5FD) }, // 5. Soft Lavender
        { "IH",   "short Ih",  "sit, hit, in, spit, kid, if",           juce::Colour(0xBA67E8F9) }, // 6. Bright Aqua Cyan
        { "OW",   "long Oh",   "go, slow, flow, tone, cold, soul",      juce::Colour(0xBAFBBF24) }, // 7. Amber Gold
        { "AO",   "short Aw",  "stop, rock, all, call, off, dog",       juce::Colour(0xBAFB923C) }, // 8. Coral Orange
        { "UW",   "long Oo",   "blue, cool, through, true, room",       juce::Colour(0xBAC084FC) }, // 9. Orchid Purple
        { "AH",   "short Uh",  "cut, sun, un-, -ble, understandable",   juce::Colour(0xBAFDA4AF) }, // 10. Blush Pink
        { "AW",   "Ow",        "down, town, out, round, now, loud",     juce::Colour(0xBAF6D8A8) }, // 11. Warm Sand
        { "OY",   "Oy",        "boy, toy, noise, voice, joy",           juce::Colour(0xBA6EE7B7) }, // 12. Emerald Seafoam
        { "AA_R", "Ar",        "car, star, dark, hard, part",           juce::Colour(0xBAFB7185) }, // 13. Pastel Crimson
        { "AO_R", "Or",        "more, door, for, storm, floor",         juce::Colour(0xBA818CF8) }, // 14. Periwinkle Indigo
        { "ER",   "Er",        "bird, turn, her, der, word, work",      juce::Colour(0xBA94A3B8) }  // 15. Sage Slate
    };
    return catalog;
}

const VowelSoundInfo* RhymeClassifier::findVowelSound(const juce::String& key)
{
    const auto& catalog = getVowelSoundCatalog();
    for (const auto& item : catalog)
    {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

const std::vector<juce::Colour>& RhymeClassifier::getHighlighterPalette()
{
    static std::vector<juce::Colour> palette;
    if (palette.empty())
    {
        const auto& catalog = getVowelSoundCatalog();
        for (const auto& item : catalog)
            palette.push_back(item.defaultColour);
    }
    return palette;
}

juce::Colour RhymeClassifier::getVowelColour(const juce::String& vowelKey) const
{
    auto it = customColours.find(vowelKey.toStdString());
    if (it != customColours.end())
        return it->second;

    if (const auto* info = findVowelSound(vowelKey))
        return info->defaultColour;

    return juce::Colour(0xBA93C5FD);
}

void RhymeClassifier::setVowelColour(const juce::String& vowelKey, const juce::Colour& colour)
{
    customColours[vowelKey.toStdString()] = colour;
}

void RhymeClassifier::resetVowelColoursToDefaults()
{
    customColours.clear();
}

void RhymeClassifier::setCustomVowelColours(const std::unordered_map<std::string, juce::Colour>& map)
{
    customColours = map;
}

static juce::String cleanStringForPhonetics(const juce::String& str)
{
    juce::String s = str.trim().toLowerCase();
    juce::String clean;
    for (int i = 0; i < s.length(); ++i)
    {
        juce::juce_wchar c = s[i];
        if (juce::CharacterFunctions::isLetter(c))
            clean += c;
    }
    return clean;
}

juce::String RhymeClassifier::extractRhymeKey(const juce::String& rawSyllable)
{
    return extractRhymeKeyWithContext(rawSyllable, {}, {});
}

juce::String RhymeClassifier::extractRhymeKeyWithContext(const juce::String& rawSyllable,
                                                         const juce::String& prevSyllable,
                                                         const juce::String& nextSyllable)
{
    juce::String s = rawSyllable.trim().toLowerCase();
    if (s.isEmpty())
        return {};

    // Non-terminal syllable ending in hyphen (e.g. "re-", "po-") is not a terminal rhyme
    if (s.endsWith("-"))
        return {};

    // If squished cell contains multiple words, rhyme the final word
    int lastSpace = s.lastIndexOfChar(' ');
    if (lastSpace >= 0)
        s = s.substring(lastSpace + 1).trim();

    juce::String clean = cleanStringForPhonetics(s);
    if (clean.isEmpty())
        return {};

    juce::String prevClean = cleanStringForPhonetics(prevSyllable);
    juce::String nextClean = cleanStringForPhonetics(nextSyllable);

    // 1. Context-aware resolution for ambiguous syllables like "a"
    if (clean == "a")
    {
        // "a" followed by "-ble" (e.g. "understandable", "readable", "capable")
        if (nextClean == "ble" || nextClean.startsWith("ble") ||
            nextClean == "tion" || nextClean == "tive" || nextClean == "cal" || nextClean == "ly")
        {
            return "AH"; // Unstressed schwa / "short Uh"
        }
        // "a" preceded by a stem ending like "stand", "read", "cap"
        if (prevClean == "stand" || prevClean == "understand" || prevClean == "read" || prevClean == "cap")
        {
            return "AH";
        }
        // Standalone isolated 'a' or letter 'A'
        return "EY"; // "long Ay"
    }

    // Common grammatical and syllabic endings
    if (clean == "ble" || clean == "ple" || clean == "dle" || clean == "tle" ||
        clean == "gle" || clean == "cle" || clean == "fle")
    {
        return "AH"; // "short Uh" (/əl/)
    }
    if (clean.endsWith("tion") || clean.endsWith("sion") || clean.endsWith("cion"))
    {
        return "AH"; // "short Uh" (/ʃən/)
    }
    if (clean == "der" || clean == "ter" || clean == "per" || clean == "ber" ||
        clean == "ger" || clean == "ver" || clean == "mer" || clean == "ner")
    {
        return "ER"; // "Er"
    }
    if (clean == "un")
    {
        return "AH"; // "short Uh"
    }

    // Standalone "I" / "i" and common "I-" contractions are always [AY] ("long Eye")
    if (clean == "i" || clean == "im" || clean == "ive" || clean == "id" ||
        s == "i'm" || s == "i've" || s == "i'd" || s == "i'll")
    {
        return "AY";
    }

    // 2. Comprehensive CMUDict phonetic dictionary (maps words to pure vowel sounds)
    static const std::unordered_map<std::string, const char*> cmuDict = {
        // [UW] phoneme ("long Oo")
        { "to", "UW" }, { "too", "UW" }, { "two", "UW" }, { "do", "UW" }, { "you", "UW" },
        { "who", "UW" }, { "through", "UW" }, { "true", "UW" }, { "blue", "UW" }, { "clue", "UW" },
        { "sue", "UW" }, { "due", "UW" }, { "new", "UW" }, { "knew", "UW" }, { "flew", "UW" },
        { "crew", "UW" }, { "grew", "UW" }, { "threw", "UW" }, { "shoe", "UW" }, { "lupe", "UW" },
        { "cool", "UW" }, { "pool", "UW" }, { "rule", "UW" }, { "fool", "UW" }, { "school", "UW" },
        { "room", "UW" }, { "boom", "UW" }, { "gloom", "UW" }, { "shoot", "UW" }, { "boot", "UW" },
        { "root", "UW" }, { "move", "UW" }, { "prove", "UW" }, { "lose", "UW" }, { "choose", "UW" },

        // [OW] phoneme ("long Oh")
        { "so", "OW" }, { "no", "OW" }, { "go", "OW" }, { "pro", "OW" }, { "flow", "OW" },
        { "grow", "OW" }, { "slow", "OW" }, { "show", "OW" }, { "know", "OW" }, { "though", "OW" },
        { "glow", "OW" }, { "blow", "OW" }, { "throw", "OW" }, { "foe", "OW" }, { "toe", "OW" },
        { "row", "OW" }, { "low", "OW" }, { "stone", "OW" }, { "bone", "OW" }, { "cone", "OW" },
        { "tone", "OW" }, { "zone", "OW" }, { "home", "OW" }, { "dome", "OW" }, { "roam", "OW" },
        { "foam", "OW" }, { "cold", "OW" }, { "hold", "OW" }, { "bold", "OW" }, { "gold", "OW" },
        { "soul", "OW" }, { "role", "OW" }, { "hole", "OW" }, { "pole", "OW" }, { "sole", "OW" },
        { "road", "OW" }, { "load", "OW" }, { "coat", "OW" }, { "boat", "OW" }, { "throat", "OW" },

        // [EY] phoneme ("long Ay")
        { "day", "EY" }, { "way", "EY" }, { "say", "EY" }, { "may", "EY" }, { "play", "EY" },
        { "stay", "EY" }, { "away", "EY" }, { "pay", "EY" }, { "they", "EY" }, { "weigh", "EY" },
        { "lay", "EY" }, { "gray", "EY" }, { "grey", "EY" }, { "hey", "EY" }, { "okay", "EY" },
        { "cake", "EY" }, { "take", "EY" }, { "make", "EY" }, { "break", "EY" }, { "shake", "EY" },
        { "wake", "EY" }, { "lake", "EY" }, { "fake", "EY" }, { "snake", "EY" }, { "steak", "EY" },
        { "place", "EY" }, { "pace", "EY" }, { "face", "EY" }, { "space", "EY" }, { "race", "EY" },
        { "case", "EY" }, { "base", "EY" }, { "grace", "EY" }, { "chase", "EY" }, { "taste", "EY" },
        { "waste", "EY" }, { "game", "EY" }, { "name", "EY" }, { "fame", "EY" }, { "came", "EY" },
        { "same", "EY" }, { "late", "EY" }, { "wait", "EY" }, { "rate", "EY" }, { "gate", "EY" },
        { "hate", "EY" }, { "date", "EY" }, { "state", "EY" }, { "great", "EY" }, { "straight", "EY" },
        { "rain", "EY" }, { "pain", "EY" }, { "train", "EY" }, { "gain", "EY" }, { "main", "EY" },
        { "plain", "EY" }, { "chain", "EY" }, { "drain", "EY" }, { "stain", "EY" }, { "brain", "EY" },
        { "pa", "EY" }, { "fra", "EY" },

        // [IY] phoneme ("long Ee")
        { "me", "IY" }, { "see", "IY" }, { "be", "IY" }, { "we", "IY" }, { "free", "IY" },
        { "tree", "IY" }, { "she", "IY" }, { "he", "IY" }, { "glee", "IY" }, { "key", "IY" },
        { "knee", "IY" }, { "sea", "IY" }, { "tea", "IY" }, { "flea", "IY" }, { "agree", "IY" },
        { "beat", "IY" }, { "heat", "IY" }, { "meat", "IY" }, { "meet", "IY" }, { "feet", "IY" },
        { "sheet", "IY" }, { "street", "IY" }, { "sweet", "IY" }, { "treat", "IY" }, { "fleet", "IY" },
        { "deep", "IY" }, { "keep", "IY" }, { "sleep", "IY" }, { "weep", "IY" }, { "sweep", "IY" },
        { "creep", "IY" }, { "leap", "IY" }, { "reap", "IY" }, { "cheap", "IY" }, { "heap", "IY" },
        { "feel", "IY" }, { "deal", "IY" }, { "real", "IY" }, { "steal", "IY" }, { "steel", "IY" },
        { "wheel", "IY" }, { "heal", "IY" }, { "meal", "IY" }, { "seal", "IY" }, { "peel", "IY" },
        { "read", "IY" }, { "lead", "IY" }, { "seed", "IY" }, { "feed", "IY" }, { "need", "IY" },
        { "speed", "IY" }, { "weed", "IY" }, { "bleed", "IY" }, { "breed", "IY" }, { "creed", "IY" },
        { "means", "IY" }, { "beans", "IY" }, { "scenes", "IY" }, { "green", "IY" }, { "clean", "IY" },
        { "ry", "IY" },

        // [AY] phoneme ("long Eye")
        { "i", "AY" }, { "im", "AY" }, { "ive", "AY" }, { "id", "AY" }, { "hi", "AY" }, { "pi", "AY" }, { "bi", "AY" },
        { "wild", "AY" }, { "child", "AY" },
        { "my", "AY" }, { "fly", "AY" }, { "high", "AY" }, { "by", "AY" }, { "buy", "AY" },
        { "die", "AY" }, { "lie", "AY" }, { "tie", "AY" }, { "pie", "AY" }, { "try", "AY" },
        { "why", "AY" }, { "sky", "AY" }, { "cry", "AY" }, { "dry", "AY" }, { "guy", "AY" },
        { "eye", "AY" }, { "spy", "AY" }, { "fry", "AY" }, { "shy", "AY" }, { "sigh", "AY" },
        { "night", "AY" }, { "right", "AY" }, { "might", "AY" }, { "light", "AY" }, { "sight", "AY" },
        { "bright", "AY" }, { "tight", "AY" }, { "fight", "AY" }, { "white", "AY" }, { "bite", "AY" },
        { "kite", "AY" }, { "quite", "AY" }, { "height", "AY" }, { "flight", "AY" }, { "slight", "AY" },
        { "time", "AY" }, { "rhyme", "AY" }, { "climb", "AY" }, { "prime", "AY" }, { "dime", "AY" },
        { "crime", "AY" }, { "chime", "AY" }, { "lime", "AY" }, { "slime", "AY" },
        { "line", "AY" }, { "shine", "AY" }, { "mine", "AY" }, { "fine", "AY" }, { "sign", "AY" },
        { "wine", "AY" }, { "nine", "AY" }, { "pine", "AY" }, { "vine", "AY" }, { "define", "AY" },
        { "life", "AY" }, { "knife", "AY" }, { "strife", "AY" }, { "wife", "AY" }, { "rife", "AY" },
        { "like", "AY" }, { "strike", "AY" }, { "mike", "AY" }, { "bike", "AY" }, { "hike", "AY" },
        { "mind", "AY" }, { "find", "AY" }, { "kind", "AY" }, { "blind", "AY" }, { "grind", "AY" },
        { "fire", "AY" }, { "wire", "AY" }, { "tire", "AY" }, { "hire", "AY" }, { "desire", "AY" },

        // [AW] phoneme ("Ow")
        { "down", "AW" }, { "town", "AW" }, { "brown", "AW" }, { "crown", "AW" }, { "drown", "AW" },
        { "clown", "AW" }, { "frown", "AW" }, { "noun", "AW" }, { "gown", "AW" },
        { "sound", "AW" }, { "round", "AW" }, { "ground", "AW" }, { "found", "AW" }, { "bound", "AW" },
        { "hound", "AW" }, { "mound", "AW" }, { "pound", "AW" }, { "wound", "AW" },
        { "out", "AW" }, { "shout", "AW" }, { "doubt", "AW" }, { "about", "AW" }, { "scout", "AW" },
        { "sprout", "AW" }, { "rout", "AW" }, { "pout", "AW" },
        { "now", "AW" }, { "how", "AW" }, { "cow", "AW" }, { "bow", "AW" }, { "plow", "AW" },
        { "loud", "AW" }, { "proud", "AW" }, { "cloud", "AW" }, { "crowd", "AW" },

        // [AO] phoneme ("short Aw" / "All")
        { "all", "AO" }, { "call", "AO" }, { "fall", "AO" }, { "ball", "AO" }, { "wall", "AO" },
        { "tall", "AO" }, { "small", "AO" }, { "hall", "AO" }, { "stall", "AO" }, { "mall", "AO" },
        { "stop", "AO" }, { "drop", "AO" }, { "top", "AO" }, { "pop", "AO" }, { "hop", "AO" },
        { "crop", "AO" }, { "shop", "AO" }, { "prop", "AO" }, { "cop", "AO" }, { "mop", "AO" },
        { "rock", "AO" }, { "block", "AO" }, { "shock", "AO" }, { "lock", "AO" }, { "clock", "AO" },
        { "knock", "AO" }, { "stock", "AO" }, { "dock", "AO" }, { "mock", "AO" }, { "box", "AO" },
        { "off", "AO" }, { "dog", "AO" }, { "log", "AO" }, { "fog", "AO" }, { "lost", "AO" },
        { "cost", "AO" }, { "frost", "AO" }, { "toss", "AO" }, { "boss", "AO" }, { "loss", "AO" },
        { "law", "AO" }, { "raw", "AO" }, { "saw", "AO" }, { "draw", "AO" }, { "claw", "AO" },
        { "flaw", "AO" }, { "straw", "AO" }, { "cause", "AO" }, { "pause", "AO" }, { "sor", "AO" },

        // [EH] phoneme ("short Eh")
        { "set", "EH" }, { "let", "EH" }, { "get", "EH" }, { "wet", "EH" }, { "met", "EH" },
        { "bet", "EH" }, { "net", "EH" }, { "yet", "EH" }, { "pet", "EH" }, { "debt", "EH" },
        { "threat", "EH" }, { "forget", "EH" }, { "sweat", "EH" }, { "reset", "EH" },
        { "bed", "EH" }, { "red", "EH" }, { "dead", "EH" }, { "head", "EH" }, { "fed", "EH" },
        { "led", "EH" }, { "said", "EH" }, { "bread", "EH" }, { "dread", "EH" }, { "spread", "EH" },
        { "thread", "EH" }, { "shed", "EH" }, { "fled", "EH" },
        { "check", "EH" }, { "wreck", "EH" }, { "neck", "EH" }, { "deck", "EH" }, { "tech", "EH" },
        { "pen", "EH" }, { "men", "EH" }, { "ten", "EH" }, { "den", "EH" }, { "when", "EH" },
        { "then", "EH" }, { "step", "EH" }, { "rep", "EH" }, { "prep", "EH" },

        // [AE] phoneme ("short Ah")
        { "stand", "AE" }, { "hand", "AE" }, { "land", "AE" }, { "band", "AE" }, { "grand", "AE" },
        { "brand", "AE" }, { "sand", "AE" }, { "understand", "AE" }, { "and", "AE" },
        { "man", "AE" }, { "can", "AE" }, { "plan", "AE" }, { "fan", "AE" }, { "pan", "AE" },
        { "ran", "AE" }, { "van", "AE" }, { "ban", "AE" }, { "clan", "AE" }, { "scan", "AE" },
        { "cat", "AE" }, { "bat", "AE" }, { "hat", "AE" }, { "fat", "AE" }, { "rat", "AE" },
        { "mat", "AE" }, { "sat", "AE" }, { "that", "AE" }, { "flat", "AE" }, { "chat", "AE" },
        { "back", "AE" }, { "black", "AE" }, { "track", "AE" }, { "pack", "AE" }, { "crack", "AE" },
        { "smack", "AE" }, { "stack", "AE" }, { "jack", "AE" }, { "attack", "AE" },
        { "rap", "AE" }, { "cap", "AE" }, { "tap", "AE" }, { "map", "AE" }, { "trap", "AE" },
        { "snap", "AE" }, { "clap", "AE" }, { "strap", "AE" }, { "wrap", "AE" },
        { "cash", "AE" }, { "flash", "AE" }, { "trash", "AE" }, { "smash", "AE" }, { "crash", "AE" },
        { "dash", "AE" }, { "clash", "AE" }, { "hands", "AE" }, { "bands", "AE" },

        // [IH] phoneme ("short Ih")
        { "it", "IH" }, { "sit", "IH" }, { "fit", "IH" }, { "hit", "IH" }, { "bit", "IH" },
        { "lit", "IH" }, { "quit", "IH" }, { "shit", "IH" }, { "spit", "IH" }, { "split", "IH" },
        { "in", "IH" }, { "win", "IH" }, { "pin", "IH" }, { "spin", "IH" }, { "twin", "IH" },
        { "skin", "IH" }, { "chin", "IH" }, { "grin", "IH" }, { "begin", "IH" },
        { "is", "IH" }, { "his", "IH" }, { "this", "IH" }, { "miss", "IH" }, { "kiss", "IH" },
        { "bliss", "IH" }, { "diss", "IH" }, { "abyss", "IH" },
        { "if", "IH" }, { "with", "IH" }, { "give", "IH" }, { "live", "IH" },
        { "kid", "IH" }, { "did", "IH" }, { "hid", "IH" }, { "rid", "IH" }, { "mid", "IH" },
        { "lid", "IH" }, { "grid", "IH" }, { "slide", "AY" },
        { "ming", "IH" }, { "saying", "IH" }, { "thing", "IH" }, { "sing", "IH" }, { "king", "IH" },
        { "ring", "IH" }, { "bring", "IH" }, { "wing", "IH" }, { "spring", "IH" },

        // [AH] phoneme ("short Uh" / Schwa)
        { "cut", "AH" }, { "but", "AH" }, { "shut", "AH" }, { "strut", "AH" }, { "what", "AH" },
        { "nut", "AH" }, { "gut", "AH" }, { "rut", "AH" }, { "hut", "AH" },
        { "sun", "AH" }, { "fun", "AH" }, { "run", "AH" }, { "gun", "AH" }, { "one", "AH" },
        { "done", "AH" }, { "won", "AH" }, { "son", "AH" }, { "ton", "AH" }, { "none", "AH" },
        { "up", "AH" }, { "cup", "AH" }, { "pup", "AH" }, { "sub", "AH" }, { "club", "AH" },
        { "rub", "AH" }, { "hub", "AH" }, { "drugs", "AH" }, { "thug", "AH" }, { "plug", "AH" },
        { "hug", "AH" }, { "bug", "AH" }, { "rug", "AH" }, { "mug", "AH" },
        { "love", "AH" }, { "above", "AH" }, { "dove", "AH" }, { "glove", "AH" },
        { "come", "AH" }, { "some", "AH" }, { "drum", "AH" }, { "hum", "AH" }, { "sum", "AH" },
        { "blood", "AH" }, { "flood", "AH" }, { "mud", "AH" }, { "bud", "AH" },
        { "put", "AH" }, { "foot", "AH" }, { "look", "AH" }, { "book", "AH" }, { "took", "AH" },
        { "cook", "AH" }, { "hook", "AH" }, { "shook", "AH" },

        // [OY] phoneme ("Oy")
        { "boy", "OY" }, { "toy", "OY" }, { "joy", "OY" }, { "coy", "OY" }, { "decoy", "OY" },
        { "noise", "OY" }, { "voice", "OY" }, { "choice", "OY" }, { "rejoice", "OY" },
        { "coin", "OY" }, { "join", "OY" }, { "point", "OY" }, { "joint", "OY" },

        // [AA_R] phoneme ("Ar")
        { "car", "AA_R" }, { "star", "AA_R" }, { "far", "AA_R" }, { "bar", "AA_R" }, { "scar", "AA_R" },
        { "hard", "AA_R" }, { "dark", "AA_R" }, { "park", "AA_R" }, { "spark", "AA_R" }, { "mark", "AA_R" },
        { "part", "AA_R" }, { "heart", "AA_R" }, { "start", "AA_R" }, { "art", "AA_R" }, { "smart", "AA_R" },
        { "chart", "AA_R" }, { "dart", "AA_R" }, { "guard", "AA_R" }, { "yard", "AA_R" },

        // [AO_R] phoneme ("Or")
        { "more", "AO_R" }, { "door", "AO_R" }, { "floor", "AO_R" }, { "four", "AO_R" }, { "for", "AO_R" },
        { "pour", "AO_R" }, { "roar", "AO_R" }, { "soar", "AO_R" }, { "shore", "AO_R" }, { "core", "AO_R" },
        { "score", "AO_R" }, { "store", "AO_R" }, { "before", "AO_R" }, { "ignore", "AO_R" },
        { "form", "AO_R" }, { "storm", "AO_R" }, { "norm", "AO_R" }, { "warm", "AO_R" }, { "swarm", "AO_R" },
        { "board", "AO_R" }, { "lord", "AO_R" }, { "cord", "AO_R" }, { "sword", "AO_R" }, { "ward", "AO_R" },

        // [ER] phoneme ("Er")
        { "bird", "ER" }, { "turn", "ER" }, { "burn", "ER" }, { "learn", "ER" }, { "earn", "ER" },
        { "her", "ER" }, { "per", "ER" }, { "word", "ER" }, { "work", "ER" }, { "first", "ER" },
        { "thirst", "ER" }, { "burst", "ER" }, { "worst", "ER" }, { "worth", "ER" }, { "birth", "ER" },
        { "earth", "ER" }, { "girl", "ER" }, { "twirl", "ER" }, { "swirl", "ER" }, { "pearl", "ER" },
        { "hurt", "ER" }, { "dirt", "ER" }, { "shirt", "ER" }, { "flirt", "ER" }, { "skirt", "ER" },
        { "sir", "ER" }, { "fur", "ER" }, { "blur", "ER" }, { "stir", "ER" }, { "cure", "UW" },
        { "were", "ER" }
    };

    auto it = cmuDict.find(clean.toStdString());
    if (it != cmuDict.end())
        return juce::String(it->second);

    // 3. Algorithmic Phonetic Vowel Sound Classification (ignoring consonants)
    int len = clean.length();

    // R-controlled checks
    if (clean.contains("ar"))
        return "AA_R";
    if (clean.contains("or") || clean.contains("ore") || clean.contains("oor") || clean.contains("oar"))
        return "AO_R";
    if (clean.contains("er") || clean.contains("ir") || clean.contains("ur") || clean.contains("ear") || clean.startsWith("wor"))
        return "ER";

    // Diphthongs
    if (clean.contains("ow") || clean.contains("ou"))
        return "AW";
    if (clean.contains("oy") || clean.contains("oi"))
        return "OY";

    // Vowel digraphs
    if (clean.contains("ee") || clean.contains("ea") || clean.contains("ie"))
        return "IY";
    if (clean.contains("oo"))
        return "UW";
    if (clean.contains("ai") || clean.contains("ay") || clean.contains("eigh"))
        return "EY";
    if (clean.contains("oa") || clean.contains("oe"))
        return "OW";
    if (clean.contains("igh") || clean.endsWith("y"))
        return "AY";
    if (clean.contains("all") || clean.contains("aw") || clean.contains("au"))
        return "AO";

    // Magic-E rule: Vowel + Consonant + E at end (e.g. cake, fine, stone, tune)
    if (len >= 3 && clean.endsWith("e") &&
        !clean.endsWith("ble") && !clean.endsWith("dle") && !clean.endsWith("ple"))
    {
        juce::juce_wchar prevVowel = 0;
        for (int i = len - 2; i >= 0; --i)
        {
            juce::juce_wchar ch = clean[i];
            if (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u')
            {
                prevVowel = ch;
                break;
            }
        }
        if (prevVowel == 'a') return "EY";
        if (prevVowel == 'i') return "AY";
        if (prevVowel == 'o') return "OW";
        if (prevVowel == 'u') return "UW";
        if (prevVowel == 'e') return "IY";
    }

    // Single basic vowel fallback (inspect final vowel in syllable)
    for (int i = len - 1; i >= 0; --i)
    {
        juce::juce_wchar c = clean[i];
        if (c == 'a') return (clean.endsWith("a") ? "EY" : "AE");
        if (c == 'e') return (clean.endsWith("e") ? "IY" : "EH");
        if (c == 'i') return (clean.endsWith("i") ? "AY" : "IH");
        if (c == 'y') return "AY";
        if (c == 'o') return (clean.endsWith("o") ? "OW" : "AO");
        if (c == 'u') return (clean.endsWith("u") ? "UW" : "AH");
    }

    return "AH";
}

void RhymeClassifier::updateRhymeMap(const std::vector<juce::String>& allSyllables)
{
    keyToColour.clear();
    if (colorMode != ColorMode::Rhymes || allSyllables.empty())
        return;

    std::unordered_map<std::string, int> freq;
    std::vector<std::string> orderedKeys;

    for (const auto& s : allSyllables)
    {
        juce::String k = extractRhymeKey(s);
        if (k.isNotEmpty())
        {
            std::string stdK = k.toStdString();
            if (freq[stdK] == 0)
                orderedKeys.push_back(stdK);
            freq[stdK]++;
        }
    }

    for (const auto& k : orderedKeys)
    {
        if (freq[k] >= 2)
        {
            keyToColour[k] = getVowelColour(k);
        }
    }
}

void RhymeClassifier::updateRhymeMapWithContext(const std::vector<std::vector<juce::String>>& lineSyllables,
                                                const std::set<std::pair<int, int>>& hiddenCells)
{
    keyToColour.clear();
    if (colorMode != ColorMode::Rhymes || lineSyllables.empty())
        return;

    std::unordered_map<std::string, int> freq;
    std::vector<std::string> orderedKeys;

    int numLines = (int)lineSyllables.size();
    for (int b = 0; b < numLines; ++b)
    {
        const auto& line = lineSyllables[b];
        int count = (int)line.size();
        for (int i = 0; i < count; ++i)
        {
            if (hiddenCells.find({ b, i }) != hiddenCells.end())
                continue; // Exclude hidden cell from rhyme frequency calculation

            juce::String prev = (i > 0) ? line[i - 1] : juce::String();
            juce::String next = (i < count - 1) ? line[i + 1] : juce::String();
            juce::String k = extractRhymeKeyWithContext(line[i], prev, next);

            if (k.isNotEmpty())
            {
                std::string stdK = k.toStdString();
                if (freq[stdK] == 0)
                    orderedKeys.push_back(stdK);
                freq[stdK]++;
            }
        }
    }

    for (const auto& k : orderedKeys)
    {
        if (freq[k] >= 2)
        {
            keyToColour[k] = getVowelColour(k);
        }
    }
}

void RhymeClassifier::cycleColorMode() noexcept
{
    if (colorMode == ColorMode::Off)
        colorMode = ColorMode::Rhymes;
    else if (colorMode == ColorMode::Rhymes)
        colorMode = ColorMode::Repeats;
    else
        colorMode = ColorMode::Off;
}

juce::String RhymeClassifier::cleanSyllableText(const juce::String& raw)
{
    juce::String s = raw.trim().toLowerCase();
    juce::String clean;
    clean.preallocateBytes(s.length());
    for (auto c : s)
    {
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
        {
            clean << c;
        }
    }
    return clean;
}

void RhymeClassifier::updateRepetitionMap(const std::vector<DocumentSyllable>& allSyllables,
                                          const std::set<std::pair<int, int>>& hiddenCells)
{
    repeatCellColours.clear();
    cellToRepeatSpan.clear();

    if (colorMode != ColorMode::Repeats || (int)allSyllables.size() < minRepeatLength)
        return;

    struct Token
    {
        int bar;
        int sylIndex;
        juce::String clean;
    };

    std::vector<Token> tokens;
    tokens.reserve(allSyllables.size());

    for (const auto& s : allSyllables)
    {
        juce::String c = cleanSyllableText(s.text);
        if (c.isNotEmpty())
        {
            tokens.push_back({ s.bar, s.sylIndex, c });
        }
    }

    int M = (int)tokens.size();
    if (M < minRepeatLength)
        return;

    const auto& palette = getHighlighterPalette();
    int nextColorIdx = 0;
    std::unordered_map<std::string, juce::Colour> phraseColours;

    auto isInstanceHidden = [&](int startIdx, int len) -> bool
    {
        for (int m = 0; m < len; ++m)
        {
            if (hiddenCells.find({ tokens[startIdx + m].bar, tokens[startIdx + m].sylIndex }) != hiddenCells.end())
                return true;
        }
        return false;
    };

    for (int i = 0; i < M; ++i)
    {
        for (int j = i + 1; j < M; ++j)
        {
            // Skip matches outside query line distance range
            int lineDistance = tokens[j].bar - tokens[i].bar;
            if (lineDistance > maxRepeatLineDistance)
                break;

            // Skip non-maximal left extensions
            if (i > 0 && tokens[i - 1].clean == tokens[j - 1].clean)
                continue;

            int L = 0;
            while (j + L < M && i + L < j && tokens[i + L].clean == tokens[j + L].clean)
            {
                L++;
            }

            if (L >= minRepeatLength)
            {
                // Record the candidate sequence spans regardless of hidden state
                std::vector<std::pair<int, int>> spanI, spanJ;
                for (int m = 0; m < L; ++m)
                {
                    spanI.push_back({ tokens[i + m].bar, tokens[i + m].sylIndex });
                    spanJ.push_back({ tokens[j + m].bar, tokens[j + m].sylIndex });
                }
                for (int m = 0; m < L; ++m)
                {
                    cellToRepeatSpan[getCellKey(tokens[i + m].bar, tokens[i + m].sylIndex)] = spanI;
                    cellToRepeatSpan[getCellKey(tokens[j + m].bar, tokens[j + m].sylIndex)] = spanJ;
                }

                // Check if both instances are unsuppressed
                bool hiddenI = isInstanceHidden(i, L);
                bool hiddenJ = isInstanceHidden(j, L);

                if (!hiddenI && !hiddenJ)
                {
                    juce::String phraseKey;
                    for (int m = 0; m < L; ++m)
                        phraseKey << tokens[i + m].clean << "|";

                    std::string stdKey = phraseKey.toStdString();
                    juce::Colour phraseCol;

                    auto it = phraseColours.find(stdKey);
                    if (it != phraseColours.end())
                    {
                        phraseCol = it->second;
                    }
                    else
                    {
                        phraseCol = palette[nextColorIdx % palette.size()];
                        phraseColours[stdKey] = phraseCol;
                        nextColorIdx++;
                    }

                    for (int m = 0; m < L; ++m)
                    {
                        uint64_t keyI = getCellKey(tokens[i + m].bar, tokens[i + m].sylIndex);
                        uint64_t keyJ = getCellKey(tokens[j + m].bar, tokens[j + m].sylIndex);

                        if (repeatCellColours.find(keyI) == repeatCellColours.end())
                            repeatCellColours[keyI] = phraseCol;

                        if (repeatCellColours.find(keyJ) == repeatCellColours.end())
                            repeatCellColours[keyJ] = phraseCol;
                    }
                }
            }
        }
    }
}

std::vector<std::pair<int, int>> RhymeClassifier::getRepeatSequenceSpanAt(int bar, int sylIndex) const
{
    uint64_t key = getCellKey(bar, sylIndex);
    auto it = cellToRepeatSpan.find(key);
    if (it != cellToRepeatSpan.end())
        return it->second;
    return { { bar, sylIndex } };
}

juce::Colour RhymeClassifier::getHighlightForCell(int barIndex, int globalSylIndex,
                                                 const juce::String& currentText,
                                                 const juce::String& prevSyl,
                                                 const juce::String& nextSyl,
                                                 const std::set<std::pair<int, int>>& hiddenCells) const
{
    if (colorMode == ColorMode::Off)
        return juce::Colours::transparentBlack;

    if (hiddenCells.find({ barIndex, globalSylIndex }) != hiddenCells.end())
        return juce::Colours::transparentBlack;

    if (colorMode == ColorMode::Rhymes)
    {
        return getHighlightWithContext(currentText, prevSyl, nextSyl);
    }
    else if (colorMode == ColorMode::Repeats)
    {
        uint64_t key = getCellKey(barIndex, globalSylIndex);
        auto it = repeatCellColours.find(key);
        if (it != repeatCellColours.end())
            return it->second;
    }

    return juce::Colours::transparentBlack;
}

juce::Colour RhymeClassifier::getHighlightForSyllable(const juce::String& syllable) const
{
    return getHighlightWithContext(syllable, {}, {});
}

juce::Colour RhymeClassifier::getHighlightWithContext(const juce::String& syllable,
                                                    const juce::String& prev,
                                                    const juce::String& next) const
{
    if (!isEnabled())
        return juce::Colours::transparentBlack;

    juce::String key = extractRhymeKeyWithContext(syllable, prev, next);
    if (key.isEmpty())
        return juce::Colours::transparentBlack;

    auto it = keyToColour.find(key.toStdString());
    if (it != keyToColour.end())
        return it->second;

    return juce::Colours::transparentBlack;
}

} // namespace CompassCadence
