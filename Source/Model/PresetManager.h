#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace CompassCadence
{

struct MetricPreset
{
    juce::String name;
    juce::String notation;
    int beatsPerBar = 4;
};

class PresetManager
{
public:
    PresetManager()
    {
        initFactoryPresets();
        loadUserPresets();
    }

    const std::vector<MetricPreset>& getFactoryPresets() const noexcept { return factoryPresets; }
    const std::vector<MetricPreset>& getUserPresets() const noexcept { return userPresets; }

    bool saveUserPreset(const juce::String& name, const juce::String& notation, int beats = 4)
    {
        juce::String trimmedName = name.trim();
        if (trimmedName.isEmpty() || notation.trim().isEmpty())
            return false;

        // Check if existing preset with same name exists -> update it
        for (auto& p : userPresets)
        {
            if (p.name.equalsIgnoreCase(trimmedName))
            {
                p.notation = notation.trim();
                p.beatsPerBar = beats;
                saveUserPresets();
                return true;
            }
        }

        userPresets.push_back({ trimmedName, notation.trim(), beats });
        saveUserPresets();
        return true;
    }

    bool deleteUserPreset(int userIndex)
    {
        if (userIndex >= 0 && userIndex < (int)userPresets.size())
        {
            userPresets.erase(userPresets.begin() + userIndex);
            saveUserPresets();
            return true;
        }
        return false;
    }

    bool deleteUserPresetByName(const juce::String& name)
    {
        for (size_t i = 0; i < userPresets.size(); ++i)
        {
            if (userPresets[i].name.equalsIgnoreCase(name.trim()))
            {
                userPresets.erase(userPresets.begin() + (int)i);
                saveUserPresets();
                return true;
            }
        }
        return false;
    }

    void reload()
    {
        loadUserPresets();
    }

private:
    void initFactoryPresets()
    {
        factoryPresets = {
            { "Straight 16ths [4444]/4:4", "[4444]/4:4", 4 },
            { "Straight 8ths [2222]/4:4",  "[2222]/4:4", 4 },
            { "Triplets [3333]/4:4",        "[3333]/4:4", 4 },
            { "Quintuplets [5555]/4:4",     "[5555]/4:4", 4 },
            { "Sextuplets [6666]/4:4",      "[6666]/4:4", 4 },
            { "Waltz [333]/3:3",            "[333]/3:3",  3 }
        };
    }

    juce::File getPresetsFile() const
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("CompassCadence")
            .getChildFile("user_presets.json");
    }

    void loadUserPresets()
    {
        userPresets.clear();
        auto file = getPresetsFile();
        if (!file.existsAsFile())
            return;

        juce::var parsedJson;
        if (juce::JSON::parse(file.loadFileAsString(), parsedJson).wasOk() && parsedJson.isArray())
        {
            auto* arr = parsedJson.getArray();
            for (const auto& item : *arr)
            {
                if (item.isObject())
                {
                    juce::String name = item["name"].toString();
                    juce::String notat = item["notation"].toString();
                    int bpb = (int)item["beats"];
                    if (bpb <= 0) bpb = 4;
                    if (name.isNotEmpty() && notat.isNotEmpty())
                    {
                        userPresets.push_back({ name, notat, bpb });
                    }
                }
            }
        }
    }

    void saveUserPresets()
    {
        auto file = getPresetsFile();
        file.getParentDirectory().createDirectory();

        juce::Array<juce::var> arr;
        for (const auto& p : userPresets)
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty("name", p.name);
            obj->setProperty("notation", p.notation);
            obj->setProperty("beats", p.beatsPerBar);
            arr.add(juce::var(obj));
        }

        juce::String jsonStr = juce::JSON::toString(juce::var(arr), true);
        file.replaceWithText(jsonStr);
    }

    std::vector<MetricPreset> factoryPresets;
    std::vector<MetricPreset> userPresets;
};

} // namespace CompassCadence
