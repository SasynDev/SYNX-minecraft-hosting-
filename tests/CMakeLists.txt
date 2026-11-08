macro(synx_add_test test_name source_file)
    add_executable(${test_name} ${source_file})
    target_link_libraries(${test_name} PRIVATE synx_agent_lib synx_common)
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src/common/include
        ${CMAKE_SOURCE_DIR}/src/agent/include
        ${CMAKE_SOURCE_DIR}/third_party
    )
    target_compile_features(${test_name} PRIVATE cxx_std_20)
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endmacro()

synx_add_test(test_logger test_logger.cpp)
synx_add_test(test_config test_config.cpp)
synx_add_test(test_filesystem test_filesystem.cpp)
synx_add_test(test_process_jobobject test_process_jobobject.cpp)
synx_add_test(test_agent_skeleton test_agent_skeleton.cpp)
synx_add_test(test_websocket test_websocket.cpp)
synx_add_test(test_watchdog test_watchdog.cpp)
synx_add_test(test_named_pipe test_named_pipe.cpp)
synx_add_test(test_server_properties test_server_properties.cpp)
