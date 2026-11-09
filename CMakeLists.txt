cmake_minimum_required(VERSION 3.25)
project(secondary_inventory_wheel CXX)

file(GLOB COOP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/coop/*.cpp")

add_mod(secondary_inventory_wheel
    FEATURES game fmt
    # Feature .cpp modules are included by mod.cpp to share its private runtime state.
    SOURCES  src/mod.cpp ${COOP_SOURCES}
    MOD_JSON mod.json
    RES_DIR  res
)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/mod.json" WHEEL_MOD_JSON)
string(JSON WHEEL_MOD_VERSION GET "${WHEEL_MOD_JSON}" version)
target_compile_definitions(secondary_inventory_wheel PRIVATE COOP_MOD_VERSION="${WHEEL_MOD_VERSION}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mod.json")

if (WIN32)
    target_link_libraries(secondary_inventory_wheel PRIVATE ws2_32 user32)
endif ()

if (MSVC)
    target_compile_options(secondary_inventory_wheel PRIVATE /Zc:__cplusplus)
endif ()
