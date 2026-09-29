cmake_minimum_required(VERSION 3.20)

project(ww2wings LANGUAGES CXX)

# Build against the Godot 4.7 extension API (matches the installed editor).
set(GODOTCPP_API_VERSION "4.7" CACHE STRING "Godot API version to generate bindings for")
set(GODOTCPP_TARGET "template_debug" CACHE STRING "godot-cpp build target")

add_subdirectory(godot-cpp)

file(GLOB_RECURSE WW2_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_library(ww2wings SHARED ${WW2_SOURCES})
target_include_directories(ww2wings PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(ww2wings PRIVATE godot-cpp)

if(MSVC)
    target_compile_options(ww2wings PRIVATE /W3 /wd4244 /wd4267 /wd4305 /utf-8)
    # godot-cpp decides whether a class registers its virtuals by comparing the
    # addresses of _bind_methods(); identical-COMDAT folding would merge the
    # empty ones and silently drop _ready/_process overrides.
    target_link_options(ww2wings PRIVATE /OPT:REF /OPT:NOICF)
endif()

set(WW2_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/game/bin")

set_target_properties(ww2wings PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    PREFIX ""
    OUTPUT_NAME "ww2wings"
    LIBRARY_OUTPUT_DIRECTORY "$<1:${WW2_OUTPUT_DIR}>"
    RUNTIME_OUTPUT_DIRECTORY "$<1:${WW2_OUTPUT_DIR}>"
    ARCHIVE_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/lib>"
    PDB_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/pdb>"
)
