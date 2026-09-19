#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "LyricDocument.h"
#include <vector>
#include <algorithm>

namespace CompassCadence
{

class SongManager
{
public:
    SongManager()
    {
        getSongsDirectory().createDirectory();
    }

    juce::File getSongsDirectory() const
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("CompassCadence")
            .getChildFile("songs");
    }

    std::vector<juce::String> getSavedSongNames() const
    {
        std::vector<juce::String> names;
        auto dir = getSongsDirectory();
        if (dir.isDirectory())
        {
            auto files = dir.findChildFiles(juce::File::findFiles, false, "*.cadence");
            for (const auto& f : files)
            {
                names.push_back(f.getFileNameWithoutExtension());
            }
            std::sort(names.begin(), names.end());
        }
        return names;
    }

    bool saveSong(const juce::String& songName, const LyricDocument& doc)
    {
        juce::String clean = songName.trim();
        if (clean.isEmpty())
            return false;

        auto dir = getSongsDirectory();
        dir.createDirectory();

        auto file = dir.getChildFile(clean + ".cadence");
        auto vt = doc.toValueTree();
        std::unique_ptr<juce::XmlElement> xml(vt.createXml());
        if (xml != nullptr)
        {
            return xml->writeTo(file);
        }
        return false;
    }

    bool loadSong(const juce::String& songName, LyricDocument& doc)
    {
        auto file = getSongsDirectory().getChildFile(songName.trim() + ".cadence");
        return loadSongFile(file, doc);
    }

    bool loadSongFile(const juce::File& file, LyricDocument& doc)
    {
        if (!file.existsAsFile())
            return false;

        std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(file);
        if (xml != nullptr)
        {
            auto vt = juce::ValueTree::fromXml(*xml);
            if (vt.isValid())
            {
                doc.fromValueTree(vt);
                return true;
            }
        }
        return false;
    }

    bool deleteSong(const juce::String& songName)
    {
        auto file = getSongsDirectory().getChildFile(songName.trim() + ".cadence");
        if (file.existsAsFile())
        {
            return file.deleteFile();
        }
        return false;
    }
};

} // namespace CompassCadence
