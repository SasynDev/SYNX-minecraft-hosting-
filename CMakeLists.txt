cmake_minimum_required(VERSION 3.20)
project(synx_hosting VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(
        /W4
        /WX-
        /permissive-
        /utf-8
        /EHsc
        /Zc:__cplusplus
        /D_CRT_SECURE_NO_WARNINGS
        /DNOMINMAX
    )
endif()

enable_testing()

add_subdirectory(src/common)
add_subdirectory(src/agent)
add_subdirectory(src/admin)
add_subdirectory(tests)

