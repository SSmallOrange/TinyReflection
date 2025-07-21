cmake_minimum_required(VERSION 3.15)

project(tinyrefl)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(TEST_PATH ${CMAKE_SOURCE_DIR}/test)

include_directories(${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add_executable(test_check_type 
#     ${TEST_PATH}/test_check_type.cpp)

# add_executable(test_get_member_count 
#     ${TEST_PATH}/test_get_member_count.cpp)

# add_executable(test_get_member_value 
#     ${TEST_PATH}/test_get_member_value.cpp)
    
# add_executable(test_get_member_string_and_type_string 
#     ${TEST_PATH}/test_get_member_string_and_type_string.cpp)

# add_executable(test_reflection_to_json 
#      ${TEST_PATH}/test_reflection_to_json.cpp)

add_executable(test_get_member_offset_map 
    ${TEST_PATH}/test_get_member_offset_map.cpp)


# add_executable(main 
#      ${TEST_PATH}/main.cpp)
