cmake_minimum_required(VERSION 3.10)

project(
    MemoryFatigue
    VERSION 1.1.0
    LANGUAGES CXX)

set(FATIGUE_PATH fatigue)
FILE(GLOB FATIGUE_SRC ${FATIGUE_PATH}/*.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 23)

# Use -DNO_COLOR to remove colors from output
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -std=c++23")

#link_libraries("-lm -ldl -lpthread")

include_directories(
    ${FATIGUE_PATH}
    include/
)

add_executable(
    fatigue
    main.cpp
    ${FATIGUE_SRC}
)

add_executable(
    demo
    demo.cpp
    ${FATIGUE_SRC}
)

# Game-specific patchers

add_executable(
    sekiropatcher
    patchers/sekiro/main.cpp
    ${FATIGUE_SRC}
)
