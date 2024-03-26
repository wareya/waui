SET(CMAKE_COLOR_MAKEFILE ON)
cmake_minimum_required(VERSION 3.15)
project(waui_demo)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wno-unused-parameter -fdiagnostics-color=always)
endif()

add_executable(bin2src src/res/bin2src.c)

file(GLOB PNG_FILES src/res/*.png)

add_custom_command(
    OUTPUT ${PROJECT_SOURCE_DIR}/src/generated/img.h
    DEPENDS ${PNG_FILES}
    DEPENDS bin2src
    COMMAND bin2src ${PROJECT_SOURCE_DIR}/src/generated/img.h ${PNG_FILES}
)

set_property(SOURCE src/main.cpp APPEND PROPERTY OBJECT_DEPENDS ${PROJECT_SOURCE_DIR}/src/generated/img.h)

include_directories(${PROJECT_SOURCE_DIR}/src/generated)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

#set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")


add_executable(
    ${PROJECT_NAME}
    WIN32
    src/main.cpp
)

if (MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE /ENTRY:mainCRTStartup)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2-static)

add_definitions(-DSDL_MAIN_HANDLED)
