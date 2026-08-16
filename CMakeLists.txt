cmake_minimum_required(VERSION 3.15)

project(tinyrefl)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/tinyrefl)
include_directories(${CMAKE_SOURCE_DIR}/tinyrefl/thirdparty)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 启用 CTest
enable_testing()

add_subdirectory(test)
