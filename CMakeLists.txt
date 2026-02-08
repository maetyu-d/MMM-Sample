cmake_minimum_required(VERSION 3.16)
project(MMMSample VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Provide JUCE_DIR via -DJUCE_DIR=/path/to/JUCE or place JUCE alongside this project.
if (NOT DEFINED JUCE_DIR)
  set(JUCE_DIR "${CMAKE_CURRENT_LIST_DIR}/JUCE")
endif()

if (EXISTS "${JUCE_DIR}/CMakeLists.txt")
  add_subdirectory(${JUCE_DIR} JUCE)
else()
  message(FATAL_ERROR "JUCE not found. Set JUCE_DIR or place JUCE in ./JUCE")
endif()

juce_add_gui_app(MBVSampler
  PRODUCT_NAME "MMM Sample"
  COMPANY_NAME "MMM"
  BUNDLE_ID "com.mmm.sample"
  VERSION "0.1.0"
)

juce_generate_juce_header(MBVSampler)

target_sources(MBVSampler PRIVATE
  Source/Main.cpp
  Source/MainComponent.cpp
  Source/AudioEngine.cpp
  Source/Sample.cpp
  Source/WarpCurve.cpp
  Source/Track.cpp
  Source/ArrangementView.cpp
  Source/TrackView.cpp
  Source/WaveformComponent.cpp
  Source/WarpCurveEditor.cpp
  Source/WarpPanel.cpp
  Source/BeatSlicerComponent.cpp
)

target_include_directories(MBVSampler PRIVATE Source)

target_compile_definitions(MBVSampler PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(MBVSampler PRIVATE
  juce::juce_audio_utils
  juce::juce_audio_devices
  juce::juce_audio_formats
  juce::juce_audio_basics
  juce::juce_dsp
  juce::juce_gui_basics
  juce::juce_gui_extra
)

if (APPLE)
  add_custom_target(run
    COMMAND "$<TARGET_FILE:MBVSampler>"
    DEPENDS MBVSampler
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Launching MMM Sample"
  )
endif()
