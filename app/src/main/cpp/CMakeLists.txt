cmake_minimum_required(VERSION 3.22.1)

project("kaosseffect")

set(CMAKE_CXX_STANDARD 17)

set(OBOE_BUILD_TESTS OFF CACHE BOOL "Build oboe tests" FORCE)
set(OBOE_BUILD_EXAMPLES OFF CACHE BOOL "Build oboe examples" FORCE)

# Fetch Google Oboe
include(FetchContent)
FetchContent_Declare(
    oboe
    GIT_REPOSITORY https://github.com/google/oboe.git
    GIT_TAG        1.8.0
)
FetchContent_MakeAvailable(oboe)

# Create a shared library
add_library(
    native-lib
    SHARED
    native-lib.cpp
    AudioEngine.cpp
    Mp3Decoder.cpp
    effects/Filter.cpp
    effects/Chorus.cpp
    effects/Bitcrusher.cpp
    effects/Flanger.cpp
    effects/Reverb.cpp
    effects/Phaser.cpp
    effects/Glitch.cpp
    effects/RingMod.cpp
)

# Link libraries
target_link_libraries(
    native-lib
    oboe
    android
    log
    mediandk
)
