#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Model/SongManager.h"

namespace CompassCadence
{

CompassCadenceAudioProcessor::CompassCadenceAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    tabs.push_back({ "Main", std::make_shared<LyricDocument>() });
    activeTabIndex = 0;
}

CompassCadenceAudioProcessor::~CompassCadenceAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout CompassCadenceAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "page", 1 }, "Page", 0, 63, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "scrollY", 1 }, "Scroll Y", 0.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "followPlayhead", 1 }, "Follow Playhead", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "rhymeHighlight", 1 }, "Rhyme Highlight", true));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "barSpacing", 1 }, "Bar Spacing",
        juce::StringArray { "Off", "4 Bars", "8 Bars", "16 Bars" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "viewMode", 1 }, "View Mode",
        juce::StringArray { "Scroll", "Pages" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "darkMode", 1 }, "Dark Mode", false));

    return { params.begin(), params.end() };
}

const juce::String CompassCadenceAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CompassCadenceAudioProcessor::acceptsMidi() const
{
    return true;
}

bool CompassCadenceAudioProcessor::producesMidi() const
{
    return false;
}

bool CompassCadenceAudioProcessor::isMidiEffect() const
{
    return false;
}

double CompassCadenceAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CompassCadenceAudioProcessor::getNumPrograms()
{
    return 1;
}

int CompassCadenceAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CompassCadenceAudioProcessor::setCurrentProgram(int)
{
}

const juce::String CompassCadenceAudioProcessor::getProgramName(int)
{
    return {};
}

void CompassCadenceAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void CompassCadenceAudioProcessor::prepareToPlay(double sampleRate, int)
{
    sampleRateCached = sampleRate;
}

void CompassCadenceAudioProcessor::releaseResources()
{
}

bool CompassCadenceAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void CompassCadenceAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // 1. If running as a plugin in a DAW, read host playhead and do not generate standalone metronome audio
    if (!isStandalone())
    {
        if (auto* ph = getPlayHead())
        {
            if (auto posOpt = ph->getPosition())
            {
                auto pos = *posOpt;

                if (auto bpmOpt = pos.getBpm())
                    currentBpm.store(*bpmOpt);

                isHostPlaying.store(pos.getIsPlaying());

                if (auto ppqOpt = pos.getPpqPosition())
                    currentPpq.store(*ppqOpt);

                if (auto timeSigOpt = pos.getTimeSignature())
                {
                    currentTimeSigNum.store(timeSigOpt->numerator);
                    currentTimeSigDenom.store(timeSigOpt->denominator);
                }
            }
        }
        return;
    }

    // 2. Standalone .exe version: handle internal transport and synthesize metronome audio
    buffer.clear();

    if (standalonePlaying)
    {
        const double bpm = standaloneBpm;
        const double bpb = (double)getLyricDocument().getNotation().getBeatsPerBar();
        const double beatsPerSec = bpm / 60.0;
        const double sr = (sampleRateCached > 1000.0) ? sampleRateCached : 44100.0;
        const double beatsPerSample = beatsPerSec / sr;
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        isHostPlaying.store(true);
        currentBpm.store(bpm);

        float* left = (numChannels > 0) ? buffer.getWritePointer(0) : nullptr;
        float* right = (numChannels > 1) ? buffer.getWritePointer(1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            double prevPpq = standalonePpq;
            standalonePpq += beatsPerSample;

            // Trigger click on integer beat boundary
            if (std::floor(prevPpq) != std::floor(standalonePpq))
            {
                int beatIdx = (int)std::floor(standalonePpq);
                int beatsPerBarInt = std::max(1, (int)bpb);
                int beatInBar = ((beatIdx % beatsPerBarInt) + beatsPerBarInt) % beatsPerBarInt;
                bool isDownbeat = (beatInBar == 0);

                if (metronomeEnabled.load())
                {
                    clickFreq = isDownbeat ? 1600.0 : 1000.0;
                    clickPhase = 0.0;
                    clickTotalSamples = (int)(sr * 0.035); // 35ms duration
                    clickSampleRemaining = clickTotalSamples;
                }
            }

            // Synthesize metronome sample
            if (clickSampleRemaining > 0 && clickTotalSamples > 0)
            {
                float progress = 1.0f - ((float)clickSampleRemaining / (float)clickTotalSamples);
                float env = std::exp(-progress * 7.0f);
                float sampleVal = (float)std::sin(clickPhase) * env * metronomeVolume.load();

                clickPhase += (juce::MathConstants<double>::twoPi * clickFreq) / sr;
                if (clickPhase >= juce::MathConstants<double>::twoPi)
                    clickPhase -= juce::MathConstants<double>::twoPi;

                clickSampleRemaining--;

                if (left != nullptr) left[i] += sampleVal;
                if (right != nullptr) right[i] += sampleVal;
            }
        }

        currentPpq.store(standalonePpq);
    }
    else
    {
        isHostPlaying.store(false);
        clickSampleRemaining = 0;
    }
}

TransportState CompassCadenceAudioProcessor::getTransportState() const noexcept
{
    TransportState st;
    st.bpm = currentBpm.load();
    st.ppqPosition = currentPpq.load();
    st.timeSigNumerator = currentTimeSigNum.load();
    st.timeSigDenominator = currentTimeSigDenom.load();
    st.isPlaying = isHostPlaying.load();
    return st;
}

void CompassCadenceAudioProcessor::setStandaloneBpm(double bpm)
{
    standaloneBpm = std::clamp(bpm, 20.0, 300.0);
    currentBpm.store(standaloneBpm);
}

void CompassCadenceAudioProcessor::toggleStandalonePlayback()
{
    standalonePlaying = !standalonePlaying;
    if (standalonePlaying)
    {
        isHostPlaying.store(true);
        currentBpm.store(standaloneBpm);

        if (metronomeEnabled.load())
        {
            const double sr = (sampleRateCached > 1000.0) ? sampleRateCached : 44100.0;
            const double bpb = (double)getLyricDocument().getNotation().getBeatsPerBar();
            int beatIdx = (int)std::floor(standalonePpq);
            int beatsPerBarInt = std::max(1, (int)bpb);
            int beatInBar = ((beatIdx % beatsPerBarInt) + beatsPerBarInt) % beatsPerBarInt;
            bool isDownbeat = (beatInBar == 0);

            clickFreq = isDownbeat ? 1600.0 : 1000.0;
            clickPhase = 0.0;
            clickTotalSamples = (int)(sr * 0.035);
            clickSampleRemaining = clickTotalSamples;
        }
    }
    else
    {
        isHostPlaying.store(false);
        clickSampleRemaining = 0;
    }
}

void CompassCadenceAudioProcessor::resetStandalonePlayback()
{
    standalonePpq = 0.0;
    currentPpq.store(0.0);
}

void CompassCadenceAudioProcessor::setManualPlayback(bool playing, double bpm)
{
    standalonePlaying = playing;
    standaloneBpm = std::clamp(bpm, 20.0, 300.0);
    isHostPlaying.store(playing);
    currentBpm.store(standaloneBpm);
    if (playing && metronomeEnabled.load())
    {
        const double sr = (sampleRateCached > 1000.0) ? sampleRateCached : 44100.0;
        const double bpb = (double)getLyricDocument().getNotation().getBeatsPerBar();
        int beatIdx = (int)std::floor(standalonePpq);
        int beatsPerBarInt = std::max(1, (int)bpb);
        int beatInBar = ((beatIdx % beatsPerBarInt) + beatsPerBarInt) % beatsPerBarInt;
        bool isDownbeat = (beatInBar == 0);

        clickFreq = isDownbeat ? 1600.0 : 1000.0;
        clickPhase = 0.0;
        clickTotalSamples = (int)(sr * 0.035);
        clickSampleRemaining = clickTotalSamples;
    }
    else if (!playing)
    {
        clickSampleRemaining = 0;
    }
}

void CompassCadenceAudioProcessor::stepManualPlayback(double deltaBeats)
{
    standalonePpq = std::max(0.0, standalonePpq + deltaBeats);
    currentPpq.store(standalonePpq);
}

LyricDocument& CompassCadenceAudioProcessor::getLyricDocument() noexcept
{
    if (tabs.empty())
        tabs.push_back({ "Main", std::make_shared<LyricDocument>() });
    if (activeTabIndex < 0 || activeTabIndex >= (int)tabs.size())
        activeTabIndex = 0;
    return *tabs[activeTabIndex].doc;
}

const LyricDocument& CompassCadenceAudioProcessor::getLyricDocument() const noexcept
{
    if (tabs.empty())
    {
        static LyricDocument fallbackDoc;
        return fallbackDoc;
    }
    int idx = std::clamp(activeTabIndex, 0, (int)tabs.size() - 1);
    return *tabs[idx].doc;
}

juce::String CompassCadenceAudioProcessor::getTabName(int index) const
{
    if (index >= 0 && index < (int)tabs.size())
        return tabs[index].name;
    return {};
}

void CompassCadenceAudioProcessor::setTabName(int index, const juce::String& name)
{
    if (index >= 0 && index < (int)tabs.size())
    {
        tabs[index].name = name;
        if (onTabsChanged)
            onTabsChanged();
    }
}

void CompassCadenceAudioProcessor::setActiveTabIndex(int index)
{
    if (index >= 0 && index < (int)tabs.size() && index != activeTabIndex)
    {
        activeTabIndex = index;
        if (onActiveTabChanged)
            onActiveTabChanged(activeTabIndex);
        if (onTabsChanged)
            onTabsChanged();
    }
}

int CompassCadenceAudioProcessor::addBlankTab(const juce::String& name)
{
    auto newDoc = std::make_shared<LyricDocument>();
    juce::String tabName = name.trim();
    if (tabName.isEmpty())
        tabName = "Song " + juce::String((int)tabs.size() + 1);

    tabs.push_back({ tabName, newDoc });
    int newIdx = (int)tabs.size() - 1;
    setActiveTabIndex(newIdx);
    if (onTabsChanged)
        onTabsChanged();
    return newIdx;
}

int CompassCadenceAudioProcessor::duplicateTab(int indexToDuplicate, const juce::String& newName)
{
    if (indexToDuplicate < 0 || indexToDuplicate >= (int)tabs.size())
        indexToDuplicate = activeTabIndex;

    auto sourceDoc = tabs[indexToDuplicate].doc;
    auto clonedDoc = std::make_shared<LyricDocument>();
    clonedDoc->fromValueTree(sourceDoc->toValueTree());

    juce::String tabName = newName.trim();
    if (tabName.isEmpty())
        tabName = tabs[indexToDuplicate].name + " (Copy)";

    tabs.push_back({ tabName, clonedDoc });
    int newIdx = (int)tabs.size() - 1;
    setActiveTabIndex(newIdx);
    if (onTabsChanged)
        onTabsChanged();
    return newIdx;
}

bool CompassCadenceAudioProcessor::closeTab(int indexToClose)
{
    if (tabs.size() <= 1)
        return false;

    if (indexToClose >= 0 && indexToClose < (int)tabs.size())
    {
        tabs.erase(tabs.begin() + indexToClose);
        if (activeTabIndex >= (int)tabs.size())
            activeTabIndex = (int)tabs.size() - 1;
        if (onActiveTabChanged)
            onActiveTabChanged(activeTabIndex);
        if (onTabsChanged)
            onTabsChanged();
        return true;
    }
    return false;
}

bool CompassCadenceAudioProcessor::loadSongIntoTab(int tabIndex, const juce::File& file)
{
    if (tabIndex < 0 || tabIndex >= (int)tabs.size() || !file.existsAsFile())
        return false;

    SongManager sm;
    if (sm.loadSongFile(file, *tabs[tabIndex].doc))
    {
        tabs[tabIndex].name = file.getFileNameWithoutExtension();
        if (onTabsChanged)
            onTabsChanged();
        return true;
    }
    return false;
}

int CompassCadenceAudioProcessor::openSongInNewTab(const juce::File& file)
{
    if (!file.existsAsFile())
        return -1;

    auto newDoc = std::make_shared<LyricDocument>();
    SongManager sm;
    if (sm.loadSongFile(file, *newDoc))
    {
        juce::String name = file.getFileNameWithoutExtension();
        tabs.push_back({ name, newDoc });
        int newIdx = (int)tabs.size() - 1;
        setActiveTabIndex(newIdx);
        if (onTabsChanged)
            onTabsChanged();
        return newIdx;
    }
    return -1;
}

bool CompassCadenceAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* CompassCadenceAudioProcessor::createEditor()
{
    return new CompassCadenceAudioProcessorEditor(*this);
}

void CompassCadenceAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ValueTree state("CompassCadenceState");

    // Add APVTS parameter state
    state.addChild(apvts.copyState(), -1, nullptr);

    // Add Tabs state
    juce::ValueTree tabsNode("Tabs");
    tabsNode.setProperty("activeTab", activeTabIndex, nullptr);

    for (int i = 0; i < (int)tabs.size(); ++i)
    {
        juce::ValueTree tNode("Tab");
        tNode.setProperty("name", tabs[i].name, nullptr);
        tNode.addChild(tabs[i].doc->toValueTree(), -1, nullptr);
        tabsNode.addChild(tNode, -1, nullptr);
    }
    state.addChild(tabsNode, -1, nullptr);

    // Also write active document as legacy CompassCadenceDocument for backwards compatibility
    state.addChild(getLyricDocument().toValueTree(), -1, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CompassCadenceAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName("CompassCadenceState"))
        {
            juce::ValueTree state = juce::ValueTree::fromXml(*xmlState);

            auto apvtsNode = state.getChildWithName(apvts.state.getType());
            if (apvtsNode.isValid())
                apvts.replaceState(apvtsNode);

            auto tabsNode = state.getChildWithName("Tabs");
            if (tabsNode.isValid() && tabsNode.getNumChildren() > 0)
            {
                tabs.clear();
                int restoredActive = tabsNode.getProperty("activeTab", 0);
                for (int i = 0; i < tabsNode.getNumChildren(); ++i)
                {
                    auto tNode = tabsNode.getChild(i);
                    juce::String name = tNode.getProperty("name", "Song " + juce::String(i + 1));
                    auto docNode = tNode.getChildWithName("CompassCadenceDocument");
                    auto doc = std::make_shared<LyricDocument>();
                    if (docNode.isValid())
                        doc->fromValueTree(docNode);
                    tabs.push_back({ name, doc });
                }
                activeTabIndex = std::clamp(restoredActive, 0, (int)tabs.size() - 1);
                if (onActiveTabChanged)
                    onActiveTabChanged(activeTabIndex);
                if (onTabsChanged)
                    onTabsChanged();
            }
            else
            {
                // Backwards-compatible load from single document node
                auto docNode = state.getChildWithName("CompassCadenceDocument");
                if (docNode.isValid())
                {
                    tabs.clear();
                    auto doc = std::make_shared<LyricDocument>();
                    doc->fromValueTree(docNode);
                    tabs.push_back({ "Main", doc });
                    activeTabIndex = 0;
                    if (onActiveTabChanged)
                        onActiveTabChanged(0);
                    if (onTabsChanged)
                        onTabsChanged();
                }
            }
        }
    }
}

} // namespace CompassCadence

// JUCE Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompassCadence::CompassCadenceAudioProcessor();
}
