cmake_minimum_required(VERSION 3.22)

project(ProgFlow VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE from GitHub
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# Fetch RubberBand for time-stretching (uses single-file compilation)
FetchContent_Declare(
    rubberband
    GIT_REPOSITORY https://github.com/breakfastquay/rubberband.git
    GIT_TAG v3.3.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(rubberband)

# RubberBand include path
set(RUBBERBAND_INCLUDE_DIR ${rubberband_SOURCE_DIR})

# Accelerate framework for RubberBand on macOS
if(APPLE)
    find_library(ACCELERATE_FRAMEWORK Accelerate)
endif()

# Define the application
juce_add_gui_app(ProgFlow
    PRODUCT_NAME "ProgFlow"
    COMPANY_NAME "ProgFlow"
    BUNDLE_ID "com.progflow.app"
    ICON_BIG ""
    ICON_SMALL ""
)

# Source files
target_sources(ProgFlow PRIVATE
    Source/Main.cpp
    Source/MainWindow.cpp
    Source/Audio/AudioEngine.cpp
    Source/Audio/TransportEngine.cpp
    Source/Audio/Track.cpp
    Source/Audio/MidiClip.cpp
    Source/Audio/AutomationLane.cpp
    Source/Audio/AutomationRecorder.cpp
    Source/Audio/AudioClip.cpp
    Source/Audio/AudioFileLoader.cpp
    Source/Audio/TimeStretchProcessor.cpp
    ${rubberband_SOURCE_DIR}/single/RubberBandSingle.cpp
    Source/Audio/TempoTrack.cpp
    Source/Audio/TimeSignatureTrack.cpp
    Source/Audio/MarkerTrack.cpp
    Source/Audio/Synths/SynthBase.cpp
    Source/Audio/Synths/SynthVoice.cpp
    Source/Audio/Synths/SynthFactory.cpp
    Source/Audio/Synths/AnalogSynth.cpp
    Source/Audio/Synths/FMSynth.cpp
    Source/Audio/Synths/ProSynth/ProSynth.cpp
    Source/Audio/Synths/ProSynth/ProSynthParams.cpp
    Source/Audio/Synths/ProSynth/WavetableOsc.cpp
    Source/Audio/Synths/ProSynth/ProSynthFilter.cpp
    Source/Audio/Synths/ProSynth/ProSynthLFO.cpp
    Source/Audio/Synths/ProSynth/ModMatrix.cpp
    Source/Audio/Synths/ProSynth/UnisonEngine.cpp
    Source/Audio/Synths/ProSynth/SubOscillator.cpp
    Source/Audio/Synths/ProSynth/NoiseGenerator.cpp
    Source/Audio/Synths/Sampler.cpp
    Source/Audio/Synths/SoundFontPlayer.cpp
    Source/Audio/Synths/DrumSynth.cpp
    Source/Audio/Effects/EffectBase.cpp
    Source/Audio/Effects/EffectChain.cpp
    Source/Audio/Effects/ReverbEffect.cpp
    Source/Audio/Effects/DelayEffect.cpp
    Source/Audio/Effects/ChorusEffect.cpp
    Source/Audio/Effects/DistortionEffect.cpp
    Source/Audio/Effects/CompressorEffect.cpp
    Source/Audio/Effects/SidechainCompressorEffect.cpp
    Source/Audio/Effects/EQEffect.cpp
    Source/Audio/Effects/PhaserEffect.cpp
    Source/Audio/Effects/FlangerEffect.cpp
    Source/Audio/Effects/TremoloEffect.cpp
    Source/Audio/Effects/BitcrusherEffect.cpp
    Source/Audio/Effects/LimiterEffect.cpp
    Source/Audio/Effects/GateEffect.cpp
    Source/Audio/Effects/FilterEffect.cpp
    Source/Audio/Effects/AmpSimulatorEffect.cpp
    Source/Audio/Effects/CabinetEffect.cpp
    Source/Audio/PluginHost.cpp
    Source/Project/ProjectManager.cpp
    Source/Project/ProjectSerializer.cpp
    Source/Project/AudioExporter.cpp
    Source/Project/PreferencesManager.cpp
    Source/MIDI/MidiLearnManager.cpp
    Source/Core/UndoManager.cpp
    Source/Core/Actions/TrackActions.cpp
    Source/UI/LookAndFeel.cpp
    Source/UI/TransportBar.cpp
    Source/UI/WelcomeScreen.cpp
    Source/UI/VirtualKeyboardPanel.cpp
    Source/UI/ToastManager.cpp
    Source/UI/Common/RotaryKnob.cpp
    Source/UI/Common/VerticalMeter.cpp
    Source/UI/Common/WaveSelector.cpp
    Source/UI/Common/SectionPanel.cpp
    Source/UI/Common/GlassPanel.cpp
    Source/UI/Common/CardPanel.cpp
    Source/UI/Common/EnvelopeVisualizer.cpp
    Source/UI/Common/ResizablePanel.cpp
    Source/UI/Tracks/TrackHeader.cpp
    Source/UI/Tracks/TrackHeaderPanel.cpp
    Source/UI/Mixer/ChannelStrip.cpp
    Source/UI/Mixer/MixerPanel.cpp
    Source/UI/Synths/SynthEditorBase.h
    Source/UI/Synths/SynthEditorBase.cpp
    Source/UI/Synths/AnalogSynthEditor.cpp
    Source/UI/Synths/FMSynthEditor.cpp
    Source/UI/Synths/ProSynthEditor.cpp
    Source/UI/Synths/SamplerEditor.cpp
    Source/UI/Synths/SoundFontPlayerEditor.cpp
    Source/UI/Synths/DrumSynthEditor.cpp
    Source/UI/Timeline/TimelinePanel.cpp
    Source/UI/Timeline/TimeRuler.cpp
    Source/UI/Timeline/TrackLane.cpp
    Source/UI/Timeline/ClipComponent.cpp
    Source/UI/Timeline/PlayheadComponent.cpp
    Source/UI/Timeline/AutomationLaneComponent.cpp
    Source/UI/Timeline/WaveformComponent.cpp
    Source/UI/Timeline/AudioClipComponent.cpp
    Source/UI/PianoRoll/PianoRollEditor.cpp
    Source/UI/PianoRoll/PianoKeyboard.cpp
    Source/UI/PianoRoll/NoteGridComponent.cpp
    Source/UI/PianoRoll/VelocityLane.cpp
    Source/UI/Effects/EffectSlot.cpp
    Source/UI/Effects/EffectChainPanel.cpp
    Source/UI/Plugins/PluginBrowserPanel.cpp
    Source/UI/Plugins/PluginEditorWindow.cpp
    Source/UI/Dialogs/ExportDialog.cpp
    Source/UI/Dialogs/PreferencesDialog.cpp
)

# JUCE modules
target_include_directories(ProgFlow PRIVATE
    ${RUBBERBAND_INCLUDE_DIR}
)

target_compile_definitions(ProgFlow PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:ProgFlow,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:ProgFlow,JUCE_VERSION>"
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
    JUCE_MODAL_LOOPS_PERMITTED=1
    $<$<PLATFORM_ID:Windows>:NOMINMAX>
)

target_link_libraries(ProgFlow PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
    $<$<PLATFORM_ID:Darwin>:${ACCELERATE_FRAMEWORK}>
)

# Bundle SoundFont if present in Resources folder (macOS only - uses app bundle)
if(APPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/GeneralUser.sf2")
    message(STATUS "Found SoundFont: Resources/GeneralUser.sf2 - will be bundled")
    # Copy SF2 to app bundle after build
    add_custom_command(TARGET ProgFlow POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/Resources/GeneralUser.sf2"
            "$<TARGET_BUNDLE_DIR:ProgFlow>/Contents/Resources/GeneralUser.sf2"
        COMMENT "Bundling GeneralUser.sf2 SoundFont"
    )
elseif(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/GeneralUser.sf2")
    message(STATUS "Found SoundFont: Resources/GeneralUser.sf2 - will be bundled")
    # Copy SF2 next to exe on Windows
    add_custom_command(TARGET ProgFlow POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/Resources/GeneralUser.sf2"
            "$<TARGET_FILE_DIR:ProgFlow>/GeneralUser.sf2"
        COMMENT "Bundling GeneralUser.sf2 SoundFont"
    )
endif()

#==============================================================================
# Plugin target (VST3, AU)
#==============================================================================

juce_add_plugin(ProgFlowPlugin
    PRODUCT_NAME "ProgFlow"
    COMPANY_NAME "ProgFlow"
    BUNDLE_ID "com.progflow.plugin"
    PLUGIN_MANUFACTURER_CODE Prfl
    PLUGIN_CODE Prfl
    FORMATS VST3 AU Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES "Instrument" "Synth"
    AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
)

# Plugin sources - reuse most of the app sources
target_sources(ProgFlowPlugin PRIVATE
    Source/Plugin/PluginProcessor.cpp
    Source/Plugin/PluginEditor.cpp
    Source/Audio/AudioEngine.cpp
    Source/Audio/TransportEngine.cpp
    Source/Audio/Track.cpp
    Source/Audio/MidiClip.cpp
    Source/Audio/AutomationLane.cpp
    Source/Audio/AutomationRecorder.cpp
    Source/Audio/AudioClip.cpp
    Source/Audio/AudioFileLoader.cpp
    Source/Audio/TimeStretchProcessor.cpp
    ${rubberband_SOURCE_DIR}/single/RubberBandSingle.cpp
    Source/Audio/TempoTrack.cpp
    Source/Audio/TimeSignatureTrack.cpp
    Source/Audio/MarkerTrack.cpp
    Source/Audio/Synths/SynthBase.cpp
    Source/Audio/Synths/SynthVoice.cpp
    Source/Audio/Synths/SynthFactory.cpp
    Source/Audio/Synths/AnalogSynth.cpp
    Source/Audio/Synths/FMSynth.cpp
    Source/Audio/Synths/ProSynth/ProSynth.cpp
    Source/Audio/Synths/ProSynth/ProSynthParams.cpp
    Source/Audio/Synths/ProSynth/WavetableOsc.cpp
    Source/Audio/Synths/ProSynth/ProSynthFilter.cpp
    Source/Audio/Synths/ProSynth/ProSynthLFO.cpp
    Source/Audio/Synths/ProSynth/ModMatrix.cpp
    Source/Audio/Synths/ProSynth/UnisonEngine.cpp
    Source/Audio/Synths/ProSynth/SubOscillator.cpp
    Source/Audio/Synths/ProSynth/NoiseGenerator.cpp
    Source/Audio/Synths/Sampler.cpp
    Source/Audio/Synths/SoundFontPlayer.cpp
    Source/Audio/Synths/DrumSynth.cpp
    Source/Audio/Effects/EffectBase.cpp
    Source/Audio/Effects/EffectChain.cpp
    Source/Audio/Effects/ReverbEffect.cpp
    Source/Audio/Effects/DelayEffect.cpp
    Source/Audio/Effects/ChorusEffect.cpp
    Source/Audio/Effects/DistortionEffect.cpp
    Source/Audio/Effects/CompressorEffect.cpp
    Source/Audio/Effects/SidechainCompressorEffect.cpp
    Source/Audio/Effects/EQEffect.cpp
    Source/Audio/Effects/PhaserEffect.cpp
    Source/Audio/Effects/FlangerEffect.cpp
    Source/Audio/Effects/TremoloEffect.cpp
    Source/Audio/Effects/BitcrusherEffect.cpp
    Source/Audio/Effects/LimiterEffect.cpp
    Source/Audio/Effects/GateEffect.cpp
    Source/Audio/Effects/FilterEffect.cpp
    Source/Audio/Effects/AmpSimulatorEffect.cpp
    Source/Audio/Effects/CabinetEffect.cpp
    Source/MIDI/MidiLearnManager.cpp
    Source/UI/LookAndFeel.cpp
    Source/UI/TransportBar.cpp
    Source/UI/Common/RotaryKnob.cpp
    Source/UI/Common/VerticalMeter.cpp
    Source/UI/Common/WaveSelector.cpp
    Source/UI/Common/SectionPanel.cpp
    Source/UI/Common/GlassPanel.cpp
    Source/UI/Common/CardPanel.cpp
    Source/UI/Common/EnvelopeVisualizer.cpp
    Source/UI/Common/ResizablePanel.cpp
    Source/UI/Tracks/TrackHeader.cpp
    Source/UI/Tracks/TrackHeaderPanel.cpp
    Source/UI/Mixer/ChannelStrip.cpp
    Source/UI/Mixer/MixerPanel.cpp
    Source/UI/Synths/SynthEditorBase.h
    Source/UI/Synths/SynthEditorBase.cpp
    Source/UI/Synths/AnalogSynthEditor.cpp
    Source/UI/Synths/FMSynthEditor.cpp
    Source/UI/Synths/ProSynthEditor.cpp
    Source/UI/Synths/SamplerEditor.cpp
    Source/UI/Synths/SoundFontPlayerEditor.cpp
    Source/UI/Synths/DrumSynthEditor.cpp
    Source/UI/Timeline/TimelinePanel.cpp
    Source/UI/Timeline/TimeRuler.cpp
    Source/UI/Timeline/TrackLane.cpp
    Source/UI/Timeline/ClipComponent.cpp
    Source/UI/Timeline/PlayheadComponent.cpp
    Source/UI/Timeline/AutomationLaneComponent.cpp
    Source/UI/Timeline/WaveformComponent.cpp
    Source/UI/Timeline/AudioClipComponent.cpp
    Source/UI/PianoRoll/PianoRollEditor.cpp
    Source/UI/PianoRoll/PianoKeyboard.cpp
    Source/UI/PianoRoll/NoteGridComponent.cpp
    Source/UI/PianoRoll/VelocityLane.cpp
    Source/UI/Effects/EffectSlot.cpp
    Source/UI/Effects/EffectChainPanel.cpp
    Source/Project/ProjectSerializer.cpp
)

target_include_directories(ProgFlowPlugin PRIVATE
    ${RUBBERBAND_INCLUDE_DIR}
)

target_compile_definitions(ProgFlowPlugin PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    $<$<PLATFORM_ID:Windows>:NOMINMAX>
)

target_link_libraries(ProgFlowPlugin PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
    $<$<PLATFORM_ID:Darwin>:${ACCELERATE_FRAMEWORK}>
)

#==============================================================================
# Test executable
#==============================================================================

juce_add_console_app(ProgFlowTests
    PRODUCT_NAME "ProgFlow Tests"
)

target_sources(ProgFlowTests PRIVATE
    Tests/TestRunner.cpp
    Tests/AutomationLaneTests.cpp
    Tests/SoundFontPlayerTests.cpp
    Tests/DrumSynthTests.cpp
    Tests/AudioClipTests.cpp
    Tests/ArrangementTests.cpp
    Tests/StressTests.cpp
    Tests/DSPTests.cpp
    Tests/IntegrationTests.cpp
    Source/Audio/AudioEngine.cpp
    Source/Audio/TransportEngine.cpp
    Source/Audio/Track.cpp
    Source/Audio/MidiClip.cpp
    Source/Audio/AutomationLane.cpp
    Source/Audio/AutomationRecorder.cpp
    Source/Audio/AudioClip.cpp
    Source/Audio/AudioFileLoader.cpp
    Source/Audio/TimeStretchProcessor.cpp
    ${rubberband_SOURCE_DIR}/single/RubberBandSingle.cpp
    Source/Audio/TempoTrack.cpp
    Source/Audio/TimeSignatureTrack.cpp
    Source/Audio/MarkerTrack.cpp
    Source/Audio/Synths/SynthBase.cpp
    Source/Audio/Synths/SynthVoice.cpp
    Source/Audio/Synths/SynthFactory.cpp
    Source/Audio/Synths/AnalogSynth.cpp
    Source/Audio/Synths/FMSynth.cpp
    Source/Audio/Synths/ProSynth/ProSynth.cpp
    Source/Audio/Synths/ProSynth/ProSynthParams.cpp
    Source/Audio/Synths/ProSynth/WavetableOsc.cpp
    Source/Audio/Synths/ProSynth/ProSynthFilter.cpp
    Source/Audio/Synths/ProSynth/ProSynthLFO.cpp
    Source/Audio/Synths/ProSynth/ModMatrix.cpp
    Source/Audio/Synths/ProSynth/UnisonEngine.cpp
    Source/Audio/Synths/ProSynth/SubOscillator.cpp
    Source/Audio/Synths/ProSynth/NoiseGenerator.cpp
    Source/Audio/Synths/Sampler.cpp
    Source/Audio/Synths/SoundFontPlayer.cpp
    Source/Audio/Synths/DrumSynth.cpp
    Source/Audio/Effects/EffectBase.cpp
    Source/Audio/Effects/EffectChain.cpp
    Source/Audio/Effects/ReverbEffect.cpp
    Source/Audio/Effects/DelayEffect.cpp
    Source/Audio/Effects/ChorusEffect.cpp
    Source/Audio/Effects/DistortionEffect.cpp
    Source/Audio/Effects/CompressorEffect.cpp
    Source/Audio/Effects/SidechainCompressorEffect.cpp
    Source/Audio/Effects/EQEffect.cpp
    Source/Audio/Effects/PhaserEffect.cpp
    Source/Audio/Effects/FlangerEffect.cpp
    Source/Audio/Effects/TremoloEffect.cpp
    Source/Audio/Effects/BitcrusherEffect.cpp
    Source/Audio/Effects/LimiterEffect.cpp
    Source/Audio/Effects/GateEffect.cpp
    Source/Audio/Effects/FilterEffect.cpp
    Source/Audio/Effects/AmpSimulatorEffect.cpp
    Source/Audio/Effects/CabinetEffect.cpp
    Source/Audio/PluginHost.cpp
)

target_include_directories(ProgFlowTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${RUBBERBAND_INCLUDE_DIR}
)

target_compile_definitions(ProgFlowTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
    $<$<PLATFORM_ID:Windows>:NOMINMAX>
)

target_link_libraries(ProgFlowTests PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    $<$<PLATFORM_ID:Darwin>:${ACCELERATE_FRAMEWORK}>
)
