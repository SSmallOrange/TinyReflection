cmake_minimum_required(VERSION 3.15)

project(tinyrefl)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(MSVC_VERSION LESS 1930)
        message(FATAL_ERROR
            "tinyrefl requires MSVC 19.30 (Visual Studio 2022) or later "
            "(earlier MSVC versions either lack /std:c++20 support or have "
            "known Concepts implementation bugs), "
            "but found MSVC_VERSION=${MSVC_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.3)
        message(FATAL_ERROR
            "tinyrefl requires GCC 11.3 or later, "
            "but found GCC ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
endif()

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(tinyrefl INTERFACE)
add_library(tinyrefl::tinyrefl ALIAS tinyrefl)
target_include_directories(tinyrefl INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/tinyrefl
    ${CMAKE_SOURCE_DIR}/tinyrefl/thirdparty
)
target_compile_features(tinyrefl INTERFACE cxx_std_20)

# 启用 CTest
enable_testing()

add_subdirectory(test)
