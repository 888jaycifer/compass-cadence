cmake_minimum_required(VERSION 3.22)

project(CompassCadence VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /FS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS")
    add_compile_options(/FS)
endif()

include(FetchContent)

# 1. Fetch or use existing JUCE 8
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../lil-apostate/build/_deps/juce-src")
    FetchContent_Declare(
        juce
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lil-apostate/build/_deps/juce-src"
    )
else()
    FetchContent_Declare(
        juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
endif()
FetchContent_MakeAvailable(juce)

# 2. Source Files
set(COMPASS_CADENCE_SOURCES
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/Model/MetricNotation.h
    Source/Model/MetricNotation.cpp
    Source/Model/PresetManager.h
    Source/Model/SongManager.h
    Source/Model/LyricDocument.h
    Source/Model/LyricDocument.cpp
    Source/Model/SyllableSplitter.h
    Source/Model/RhymeClassifier.h
    Source/Model/RhymeClassifier.cpp
    Source/UI/NotebookLookAndFeel.h
    Source/UI/NotebookLookAndFeel.cpp
    Source/UI/SyllableCellComponent.h
    Source/UI/SyllableCellComponent.cpp
    Source/UI/PulseGroupComponent.h
    Source/UI/PulseGroupComponent.cpp
    Source/UI/BarLineComponent.h
    Source/UI/BarLineComponent.cpp
    Source/UI/NotebookHeaderComponent.h
    Source/UI/NotebookHeaderComponent.cpp
    Source/UI/NotebookTabBarComponent.h
    Source/UI/NotebookTabBarComponent.cpp
    Source/UI/NotebookPageView.h
    Source/UI/NotebookPageView.cpp
)

if (EMSCRIPTEN OR COMPASS_CADENCE_BUILD_WEB)
    # -------------------------------------------------------------------------
    # WebAssembly (WASM) Target via Emscripten
    # -------------------------------------------------------------------------
    add_executable(CompassCadenceWeb
        Source/MainWeb.cpp
        ${COMPASS_CADENCE_SOURCES}
    )

    target_compile_definitions(CompassCadenceWeb PRIVATE
        JUCE_WASM=1
        JUCE_WEB=1
        COMPASS_CADENCE_BUILD_WEB=1
        JUCE_STANDALONE_APPLICATION=1
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
    )

    target_include_directories(CompassCadenceWeb PRIVATE Source)

    target_link_libraries(CompassCadenceWeb PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
    )

    if (EMSCRIPTEN)
        set(EMSCRIPTEN_LINK_FLAGS
            "-s WASM=1"
            "-s ALLOW_MEMORY_GROWTH=1"
            "-s NO_EXIT_RUNTIME=1"
            "-s USE_WEBGL2=1"
            "-s FULL_ES2=1"
            "-s MIN_WEBGL_VERSION=2"
            "-s MAX_WEBGL_VERSION=2"
            "-s FORCE_FILESYSTEM=1"
            "-idbflags"
            "-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','FS']"
            "-s EXPORTED_FUNCTIONS=['_main']"
            "-O3"
            "--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/Web/shell.html"
        )
        string(JOIN " " EMSCRIPTEN_LINK_FLAGS_STR ${EMSCRIPTEN_LINK_FLAGS})
        set_target_properties(CompassCadenceWeb PROPERTIES
            OUTPUT_NAME "index"
            SUFFIX ".html"
            LINK_FLAGS "${EMSCRIPTEN_LINK_FLAGS_STR}"
        )
    endif()

else()
    # -------------------------------------------------------------------------
    # Desktop Native Targets: VST3 + Standalone .exe (Completely Unmodified)
    # -------------------------------------------------------------------------
    juce_add_plugin(CompassCadence
        COMPANY_NAME "OpenSourceAudio"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD FALSE
        PLUGIN_MANUFACTURER_CODE "OpAu"
        PLUGIN_CODE "CmpC"
        FORMATS VST3 Standalone
        PRODUCT_NAME "Compass Cadence"
    )

    juce_generate_juce_header(CompassCadence)

    target_sources(CompassCadence PRIVATE ${COMPASS_CADENCE_SOURCES})

    target_include_directories(CompassCadence PRIVATE Source)

    if (MSVC)
        target_compile_options(CompassCadence PRIVATE /bigobj /FS)
    endif()

    target_link_libraries(CompassCadence PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce_recommended_config_flags
        juce_recommended_warning_flags
    )

    # Diagnostic Test Executable
    add_executable(TestPlugin test_runner.cpp)
    target_include_directories(TestPlugin PRIVATE Source)
    if (MSVC)
        target_compile_options(TestPlugin PRIVATE /bigobj /FS)
    endif()
    target_link_libraries(TestPlugin PRIVATE
        CompassCadence
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_audio_processors
        juce::juce_audio_utils
    )
endif()
