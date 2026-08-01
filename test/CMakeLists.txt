include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 辅助函数
function(add_tinyrefl_test name)
    add_executable(${name} ${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_tinyrefl_test(test_check_type)
add_tinyrefl_test(test_get_member_count)
add_tinyrefl_test(test_get_member_value)
add_tinyrefl_test(test_get_member_string_and_type_string)
add_tinyrefl_test(test_reflection_to_json)
add_tinyrefl_test(test_get_member_offset_map)
add_tinyrefl_test(test_reflection_from_json)
add_tinyrefl_test(test_json_serialization_bugs)
add_tinyrefl_test(test_roundtrip_bugs)
add_tinyrefl_test(test_edge_cases2)

# 性能测试
add_executable(test_pref_reflection ${CMAKE_CURRENT_SOURCE_DIR}/test_pref_reflection.cpp)
# add_test(NAME test_pref_reflection COMMAND test_pref_reflection)
