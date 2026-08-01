cmake_minimum_required(VERSION 3.15)

project(tinyrefl)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(TEST_PATH ${CMAKE_SOURCE_DIR}/test)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/tinyrefl)
include_directories(${CMAKE_SOURCE_DIR}/tinyrefl/thirdparty)
include_directories(${TEST_PATH})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 启用 CTest
enable_testing()

# 测试目标
add_executable(test_check_type 
    ${TEST_PATH}/test_check_type.cpp)

add_executable(test_get_member_count 
    ${TEST_PATH}/test_get_member_count.cpp)

add_executable(test_get_member_value 
    ${TEST_PATH}/test_get_member_value.cpp)
    
add_executable(test_get_member_string_and_type_string 
    ${TEST_PATH}/test_get_member_string_and_type_string.cpp)

add_executable(test_reflection_to_json 
     ${TEST_PATH}/test_reflection_to_json.cpp)

add_executable(test_get_member_offset_map 
    ${TEST_PATH}/test_get_member_offset_map.cpp)

add_executable(test_reflection_from_json 
    ${TEST_PATH}/test_reflection_from_json.cpp)

add_executable(test_json_serialization_bugs
    ${TEST_PATH}/test_json_serialization_bugs.cpp)

add_executable(test_pref_reflection 
    ${TEST_PATH}/test_pref_reflection.cpp)

# 注册测试到 CTest
add_test(NAME test_check_type COMMAND test_check_type)
add_test(NAME test_get_member_count COMMAND test_get_member_count)
add_test(NAME test_get_member_value COMMAND test_get_member_value)
add_test(NAME test_get_member_string_and_type_string COMMAND test_get_member_string_and_type_string)
add_test(NAME test_reflection_to_json COMMAND test_reflection_to_json)
add_test(NAME test_get_member_offset_map COMMAND test_get_member_offset_map)
add_test(NAME test_reflection_from_json COMMAND test_reflection_from_json)
add_test(NAME test_json_serialization_bugs COMMAND test_json_serialization_bugs)
# 性能测试
# add_test(NAME test_pref_reflection COMMAND test_pref_reflection)
