cmake_minimum_required(VERSION 3.22)
project(pokeheartgold LANGUAGES C CXX)

# Language standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform definition
add_compile_definitions(PLATFORM_NATIVE=1)

# Compiler warnings and options
if(MSVC)
    add_compile_options(/W3 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

# Dependencies: SDL2 and OpenGL
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

# Normalize SDL2 imported target
if(TARGET SDL2::SDL2)
    set(PORT_SDL2_TARGET SDL2::SDL2)
else()
    add_library(PORT_SDL2 INTERFACE)
    target_include_directories(PORT_SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(PORT_SDL2 INTERFACE ${SDL2_LIBRARIES})
    set(PORT_SDL2_TARGET PORT_SDL2)
endif()

# Common include directories for native port
set(PORT_COMMON_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/port/include
    ${CMAKE_CURRENT_SOURCE_DIR}/port
)

# Automated testing
enable_testing()

file(GLOB_RECURSE PORT_TEST_SOURCES
    "port/tests/*.cpp"
    "port/tests/*.c"
)

file(GLOB_RECURSE PORT_HAL_SOURCES
    "port/hal/*.cpp"
    "port/hal/*.c"
    "port/display/*.cpp"
    "port/display/*.c"
    "port/input/*.cpp"
    "port/input/*.c"
    "port/audio/*.cpp"
    "port/audio/*.c"
)

if(PORT_TEST_SOURCES)
    add_executable(port_tests ${PORT_TEST_SOURCES} ${PORT_HAL_SOURCES})
    target_include_directories(port_tests PRIVATE ${PORT_COMMON_INCLUDES})
    target_link_libraries(port_tests PRIVATE ${PORT_SDL2_TARGET} OpenGL::GL)
    if(UNIX AND NOT APPLE)
        target_link_libraries(port_tests PRIVATE pthread dl m)
    endif()
    add_test(NAME port_tests COMMAND port_tests)
endif()

# Main game executable
file(GLOB_RECURSE PORT_SOURCES
    "port/*.cpp"
    "port/*.c"
)
list(FILTER PORT_SOURCES EXCLUDE REGEX "port/tests/")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/port/main.cpp")
    add_executable(pokeheartgold ${PORT_SOURCES})
    target_include_directories(pokeheartgold PRIVATE ${PORT_COMMON_INCLUDES})
    target_link_libraries(pokeheartgold PRIVATE ${PORT_SDL2_TARGET} OpenGL::GL)
    if(UNIX AND NOT APPLE)
        target_link_libraries(pokeheartgold PRIVATE pthread dl m)
    endif()
endif()

# Optional decompilation tools (requires libpng / zlib host packages)
option(BUILD_DECOMP_TOOLS "Build ROM decompilation tools" OFF)
if(BUILD_DECOMP_TOOLS)
    add_subdirectory(tools)
endif()
