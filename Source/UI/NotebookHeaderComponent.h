#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Model/LyricDocument.h"
#include "../Model/PresetManager.h"
#include "../Model/SongManager.h"
#include "../PluginProcessor.h"

namespace CompassCadence
{

class NotebookHeaderComponent : public juce::Component,
                                public juce::TextEditor::Listener,
                                public juce::ComboBox::Listener,
                                public LyricDocument::Listener
{
public:
    NotebookHeaderComponent(CompassCadenceAudioProcessor& processor, LyricDocument& doc);
    ~NotebookHeaderComponent() override;

    void updateDAWStatus(const TransportState& state);
    void updateNotationDisplay();
    void updatePageDisplay();
    void updateViewModeDisplay();
    void updateDarkModeDisplay();
    void updateStandaloneTransportDisplay();
    void refreshPresetCombo();
    void promptSavePreset();
    void promptDeletePreset();
    void promptSaveSong();
    void promptExportTxt();
    void promptExportCsv();
    void promptExportHtml();

    void setDocument(LyricDocument& newDoc);

    std::function<void(int)> onScrollByBars;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Listeners
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost(juce::TextEditor& editor) override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

    // LyricDocument::Listener
    void lyricDocumentChanged() override;
    void metricNotationChanged(const MetricNotation& newNotation) override;

private:
    void updateEditorColours();
    void updateRhymeButtonDisplay();
    void showColorModeMenu();
    void applyNotationFromText();
    void applyPreset(int presetId);

    class RhymeButtonListener;
    std::unique_ptr<RhymeButtonListener> rhymeButtonListener;

    CompassCadenceAudioProcessor& processor;
    LyricDocument* document = nullptr;
    PresetManager presetManager;
    SongManager songManager;

    juce::Label notationLabel;
    juce::TextEditor notationEditor;
    juce::ComboBox presetCombo;
    juce::TextButton savePresetBtn { "Save" };

    // Independent steppers
    juce::Label pulseStepperLabel;
    juce::TextButton decPulseBtn { "-" };
    juce::TextButton incPulseBtn { "+" };

    juce::Label beatStepperLabel;
    juce::TextButton decBeatBtn { "-" };
    juce::TextButton incBeatBtn { "+" };

    // Toggles
    juce::TextButton rhymeToggleBtn { "Rhymes: ON" };
    juce::TextButton rhymeColorsBtn { "Pal" };
    juce::TextButton followToggleBtn { "Follow DAW: ON" };
    juce::TextButton copyBtn { "Copy" };
    juce::TextButton exportBtn { "Export" };
    juce::TextButton songsBtn { "Songs" };

    // View mode, Page navigation & Theme
    juce::TextButton darkModeToggleBtn;
    juce::TextButton viewModeToggleBtn;
    juce::TextButton prevPageBtn { "<" };
    juce::Label pageLabel;
    juce::TextButton nextPageBtn { ">" };

    juce::String currentSongName = "My Song";
    juce::String currentPresetName = "My Flow";

    // DAW Status (for DAW VST3 plugin mode)
    juce::Label dawStatusLabel;

    // Standalone Transport & Metronome Controls (for .exe standalone mode)
    juce::TextButton standalonePlayBtn { "Play" };
    juce::TextButton metronomeToggleBtn { "Click: ON" };
    juce::TextButton decBpmBtn { "-" };
    juce::Label bpmLabel;
    juce::TextButton incBpmBtn { "+" };

    static constexpr float MARGIN_X = 65.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotebookHeaderComponent)
};

} // namespace CompassCadence
