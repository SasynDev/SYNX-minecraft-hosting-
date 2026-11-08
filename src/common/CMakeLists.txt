add_library(synx_common STATIC
    src/types.cpp
    src/logger.cpp
    src/config.cpp
    src/filesystem.cpp
    src/process.cpp
    src/system_info.cpp
    src/crypto.cpp
    src/websocket.cpp
    src/server_properties.cpp
)

target_include_directories(synx_common PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party>
)

target_compile_features(synx_common PUBLIC cxx_std_20)

if(WIN32)
    target_link_libraries(synx_common PUBLIC bcrypt rpcrt4 ws2_32 crypt32)
endif()
