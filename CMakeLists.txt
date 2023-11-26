SET(CMAKE_COLOR_MAKEFILE ON)
cmake_minimum_required(VERSION 3.15)
project(waui_demo)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

#set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wno-unused-parameter -fdiagnostics-color=always)
endif()

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2-static)
add_definitions(-DSDL_MAIN_HANDLED)
