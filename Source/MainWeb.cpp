#if defined(__EMSCRIPTEN__) || defined(JUCE_WASM) || defined(COMPASS_CADENCE_BUILD_WEB)

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <emscripten.h>
#include <emscripten/html5.h>

namespace CompassCadence
{

class WebAppWindow : public juce::TopLevelWindow
{
public:
    WebAppWindow(const juce::String& name, CompassCadenceAudioProcessor& proc)
        : TopLevelWindow(name, false), processor(proc)
    {
        setOpaque(true);
        editor.reset(processor.createEditor());
        if (editor != nullptr)
        {
            addAndMakeVisible(editor.get());
        }

        // Match initial browser canvas / window size
        updateCanvasSize();
        setVisible(true);

        // Register window resize callback with Emscripten
        emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true,
            [](int, const EmscriptenUiEvent*, void* userData) -> EM_BOOL
            {
                if (auto* w = static_cast<WebAppWindow*>(userData))
                {
                    w->updateCanvasSize();
                }
                return EM_TRUE;
            });
    }

    ~WebAppWindow() override
    {
        emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, false, nullptr);
        editor.reset();
    }

    void updateCanvasSize()
    {
        double cssW = 0.0, cssH = 0.0;
        emscripten_get_element_css_size("canvas", &cssW, &cssH);

        int w = (int)std::round(cssW);
        int h = (int)std::round(cssH);

        if (w <= 0 || h <= 0)
        {
            w = (int)emscripten_get_screen_width();
            h = (int)emscripten_get_screen_height();
        }

        if (w <= 0) w = 1040;
        if (h <= 0) h = 720;

        setBounds(0, 0, w, h);
    }

    void resized() override
    {
        if (editor != nullptr)
        {
            editor->setBounds(getLocalBounds());
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(NotebookLookAndFeel::getPaperColour());
    }

private:
    CompassCadenceAudioProcessor& processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebAppWindow)
};

class CompassCadenceWebApplication : public juce::JUCEApplication
{
public:
    CompassCadenceWebApplication() = default;

    const juce::String getApplicationName() override       { return "Compass Cadence"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise(const juce::String&) override
    {
        // 1. Initialize persistent browser storage (IDBFS)
        EM_ASM(
            if (typeof FS !== 'undefined' && FS.filesystems && FS.filesystems.IDBFS) {
                try {
                    FS.mkdir('/CompassCadence');
                    FS.mount(FS.filesystems.IDBFS, {}, '/CompassCadence');
                    FS.syncfs(true, function(err) {
                        if (err) console.warn("Initial IDBFS sync note:", err);
                        else console.log("Compass Cadence IDBFS storage ready.");
                    });
                } catch(e) {
                    console.warn("Storage mount note:", e);
                }
            }
        );

        // 2. Initialize Audio Processor
        processor = std::make_unique<CompassCadenceAudioProcessor>();
        processor->prepareToPlay(44100.0, 512);

        // 3. Initialize Web App GUI Window
        mainWindow = std::make_unique<WebAppWindow>(getApplicationName(), *processor);
    }

    void shutdown() override
    {
        // Sync any unsaved storage changes before shutdown
        EM_ASM(
            if (typeof FS !== 'undefined' && FS.syncfs) {
                try {
                    FS.syncfs(false, function(err) {});
                } catch(e) {}
            }
        );

        mainWindow.reset();
        processor.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override {}

private:
    std::unique_ptr<CompassCadenceAudioProcessor> processor;
    std::unique_ptr<WebAppWindow> mainWindow;
};

} // namespace CompassCadence

START_JUCE_APPLICATION(CompassCadence::CompassCadenceWebApplication)

#endif // __EMSCRIPTEN__ || JUCE_WASM
