#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Model/LyricDocument.h"
#include <atomic>

namespace CompassCadence
{

struct TransportState
{
    double bpm = 120.0;
    double ppqPosition = 0.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
    bool isPlaying = false;
    bool isLooping = false;
};

class CompassCadenceAudioProcessor : public juce::AudioProcessor
{
public:
    CompassCadenceAudioProcessor();
    ~CompassCadenceAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    struct NotebookTab
    {
        juce::String name;
        std::shared_ptr<LyricDocument> doc;
    };

    int getNumTabs() const noexcept { return (int)tabs.size(); }
    int getActiveTabIndex() const noexcept { return activeTabIndex; }
    juce::String getTabName(int index) const;
    void setTabName(int index, const juce::String& name);
    void setActiveTabIndex(int index);
    int addBlankTab(const juce::String& name = {});
    int duplicateTab(int indexToDuplicate, const juce::String& newName = {});
    bool closeTab(int indexToClose);
    bool loadSongIntoTab(int tabIndex, const juce::File& file);
    int openSongInNewTab(const juce::File& file);

    std::function<void()> onTabsChanged;
    std::function<void(int)> onActiveTabChanged;

    LyricDocument& getLyricDocument() noexcept;
    const LyricDocument& getLyricDocument() const noexcept;

    TransportState getTransportState() const noexcept;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    bool isFollowDAW() const noexcept { return followDAW.load(); }
    void setFollowDAW(bool follow) { followDAW.store(follow); }

    // Manual playback position and Metronome for Standalone mode when no DAW transport exists
    bool isStandalone() const noexcept
    {
#if defined(__EMSCRIPTEN__) || defined(JUCE_WASM) || defined(COMPASS_CADENCE_BUILD_WEB)
        return true;
#else
        return wrapperType == wrapperType_Standalone || juce::JUCEApplicationBase::isStandaloneApp() || forceStandaloneMode;
#endif
    }
    void setForceStandaloneMode(bool force) noexcept { forceStandaloneMode = force; }

    bool isMetronomeEnabled() const noexcept { return metronomeEnabled.load(); }
    void setMetronomeEnabled(bool enabled) noexcept { metronomeEnabled.store(enabled); }

    float getMetronomeVolume() const noexcept { return metronomeVolume.load(); }
    void setMetronomeVolume(float vol) noexcept { metronomeVolume.store(std::clamp(vol, 0.0f, 1.0f)); }

    double getStandaloneBpm() const noexcept { return standaloneBpm; }
    void setStandaloneBpm(double bpm);

    bool isStandalonePlaying() const noexcept { return standalonePlaying; }
    void toggleStandalonePlayback();
    void resetStandalonePlayback();

    void setManualPlayback(bool playing, double bpm = 120.0);
    void stepManualPlayback(double deltaBeats);

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::vector<NotebookTab> tabs;
    int activeTabIndex = 0;
    std::atomic<bool> followDAW { true };

    std::atomic<double> currentBpm { 120.0 };
    std::atomic<double> currentPpq { 0.0 };
    std::atomic<int> currentTimeSigNum { 4 };
    std::atomic<int> currentTimeSigDenom { 4 };
    std::atomic<bool> isHostPlaying { false };

    // Standalone fallback transport & metronome
    bool standalonePlaying = false;
    double standaloneBpm = 120.0;
    double standalonePpq = 0.0;
    double sampleRateCached = 44100.0;
    bool forceStandaloneMode = false;

    std::atomic<bool> metronomeEnabled { true };
    std::atomic<float> metronomeVolume { 0.7f };

    double clickPhase = 0.0;
    double clickFreq = 1000.0;
    int clickSampleRemaining = 0;
    int clickTotalSamples = 0;

    juce::AudioProcessorValueTreeState apvts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompassCadenceAudioProcessor)
};

} // namespace CompassCadence
