cmake_minimum_required(VERSION 3.25)
project(secondary_inventory_wheel CXX)

file(GLOB COOP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/coop/*.cpp")

include(FetchContent)
set(COOP_VOICE ON)
set(COOP_VOICE_SDL OFF)
if (WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(COOP_VOICE_SDL ON)
endif ()

if (COOP_VOICE)
    block()
        set(BUILD_SHARED_LIBS OFF)
        set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
        set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
        set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
        set(OPUS_INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
        set(OPUS_INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(coop_opus
            GIT_REPOSITORY https://github.com/xiph/opus.git
            GIT_TAG v1.5.2
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(coop_opus)
        set_target_properties(opus PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endblock()
endif ()

if (COOP_VOICE_SDL OR CMAKE_SYSTEM_NAME STREQUAL "Android")
    file(GLOB COOP_SDL3_HINTS "${DUSK_DIR}/build/*/_deps/sdl3_prebuilt-src/include"
                              "${DUSK_DIR}/build/*/_deps/sdl3-src/include")
    find_path(COOP_SDL3_INCLUDE_DIR SDL3/SDL_audio.h HINTS ${COOP_SDL3_HINTS} NO_DEFAULT_PATH)
    if (NOT COOP_SDL3_INCLUDE_DIR)
        FetchContent_Declare(coop_sdl3_headers
            URL "https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.10.tar.gz"
            DOWNLOAD_EXTRACT_TIMESTAMP FALSE)
        FetchContent_GetProperties(coop_sdl3_headers)
        if (NOT coop_sdl3_headers_POPULATED)
            if (POLICY CMP0169)
                cmake_policy(PUSH)
                cmake_policy(SET CMP0169 OLD)
            endif ()
            FetchContent_Populate(coop_sdl3_headers)
            if (POLICY CMP0169)
                cmake_policy(POP)
            endif ()
        endif ()
        set(COOP_SDL3_INCLUDE_DIR "${coop_sdl3_headers_SOURCE_DIR}/include")
    endif ()
endif ()

add_mod(secondary_inventory_wheel
    FEATURES game fmt webgpu
    # Feature .cpp modules are included by mod.cpp to share its private runtime state.
    SOURCES  src/mod.cpp ${COOP_SOURCES}
    MOD_JSON mod.json
    RES_DIR  res
)
target_include_directories(secondary_inventory_wheel PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/mod.json" WHEEL_MOD_JSON)
string(JSON WHEEL_MOD_VERSION GET "${WHEEL_MOD_JSON}" version)
target_compile_definitions(secondary_inventory_wheel PRIVATE COOP_MOD_VERSION="${WHEEL_MOD_VERSION}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mod.json")

if (COOP_VOICE)
    target_link_libraries(secondary_inventory_wheel PRIVATE opus)
    target_compile_definitions(secondary_inventory_wheel PRIVATE COOP_VOICE=1)
endif ()
if (COOP_VOICE_SDL OR CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_include_directories(secondary_inventory_wheel PRIVATE "${COOP_SDL3_INCLUDE_DIR}")
endif ()
if (COOP_VOICE_SDL)
    target_compile_definitions(secondary_inventory_wheel PRIVATE COOP_VOICE_SDL=1)
elseif (APPLE)
    target_link_libraries(secondary_inventory_wheel PRIVATE "-framework AudioToolbox" "-framework CoreFoundation")
elseif (ANDROID)
    target_link_libraries(secondary_inventory_wheel PRIVATE aaudio)
endif ()

if (WIN32)
    target_link_libraries(secondary_inventory_wheel PRIVATE ws2_32 user32)
endif ()

if (MSVC)
    target_compile_options(secondary_inventory_wheel PRIVATE /Zc:__cplusplus)
endif ()
