cmake_minimum_required(VERSION 3.20)
project(synx_admin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

add_executable(synx-admin
    src/main.cpp
    src/main_window.cpp
    src/agent_client.cpp
    src/server_wizard_dialog.cpp
    include/main_window.hpp
    include/agent_client.hpp
    include/server_wizard_dialog.hpp
    include/admin_theme.hpp
)

target_include_directories(synx-admin PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/src/common/include
)

target_link_libraries(synx-admin PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    synx_common
)

if(MSVC)
    target_link_libraries(synx-admin PRIVATE ws2_32)
    # Set Windows subsystem for native desktop application
    # Leave console or windows subsystem as desired
endif()

