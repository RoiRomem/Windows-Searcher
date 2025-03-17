cmake_minimum_required(VERSION 3.30)
project(Island)

set(CMAKE_CXX_STANDARD 26)

add_executable(Island WIN32 main.cpp Island.h
        util.cpp
        widgetData.cpp)

target_compile_options(Island PRIVATE /await)
target_link_libraries(Island PRIVATE windowsapp)