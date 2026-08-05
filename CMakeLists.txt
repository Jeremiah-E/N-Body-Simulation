cmake_minimum_required(VERSION 3.16)
project(NBody)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

find_package(SDL3 REQUIRED CONFIG PATHS "$ENV{USERPROFILE}/vendored/SDL/install")

add_executable(NBody WIN32 main.cpp)
target_link_libraries(NBody PRIVATE SDL3::SDL3)

target_compile_options(NBody PRIVATE
    $<$<AND:$<C_COMPILER_ID:MSVC>,$<NOT:$<CONFIG:Debug>>>: /O2 /fp:fast /W4 >
    $<$<AND:$<C_COMPILER_ID:MSVC>,$<CONFIG:Debug>>: /W4 >
    $<$<C_COMPILER_ID:GNU,Clang>: -O3 -ffast-math -Wall >
)

add_custom_command(TARGET NBody POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$ENV{USERPROFILE}/vendored/SDL/install/bin/SDL3.dll"
    $<TARGET_FILE_DIR:NBody>
)