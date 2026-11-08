add_library(synx_agent_lib STATIC
    src/network_manager.cpp
    src/console_manager.cpp
    src/instance_manager.cpp
    src/backup_manager.cpp
    src/server_installer.cpp
    src/watchdog.cpp
    src/named_pipe_server.cpp
    src/agent_api.cpp
    src/agent_service.cpp
)

target_include_directories(synx_agent_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/common/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party>
)

target_compile_features(synx_agent_lib PUBLIC cxx_std_20)
target_link_libraries(synx_agent_lib PUBLIC synx_common)

if(WIN32)
    target_link_libraries(synx_agent_lib PUBLIC ws2_32 winhttp)
endif()

add_executable(synx-agent
    src/main.cpp
)

target_link_libraries(synx-agent PRIVATE synx_agent_lib)
target_compile_features(synx-agent PRIVATE cxx_std_20)
