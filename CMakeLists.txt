cmake_minimum_required(VERSION 3.10)

project(
    MemoryFatigue
    VERSION 1.0
    LANGUAGES CXX)

# set(KITTYMEMORY_PATH KittyMemoryEx)
# FILE(GLOB KITTYMEMORY_SRC ${KITTYMEMORY_PATH}/*.cpp)

set(FATIGUE_PATH fatigue)
FILE(GLOB FATIGUE_SRC ${FATIGUE_PATH}/*.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 23)

# https://github.com/keystone-engine/keystone
# get keystone binaries and remove DkNO_KEYSTONE to use MemoryPatch::craeteWithAsm
# Use -DNO_COLOR to remove colors from output
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -std=c++23 -DkNO_KEYSTONE")

#link_libraries("-lm -ldl -lpthread")

include_directories(
    ${FATIGUE_PATH}
)

add_executable(
    fatigue
    main.cpp
    ${FATIGUE_SRC}
)