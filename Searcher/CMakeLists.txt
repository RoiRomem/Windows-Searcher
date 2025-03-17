cmake_minimum_required(VERSION 3.20)
project(WindowsSearcher)

set(CMAKE_CXX_STANDARD 20)

add_executable(WindowsSearcher WIN32
        main.cpp
        extraCommands.cpp
        optioneer.cpp
        styler.cpp
        Searcher.h)

# Add Windows libraries
target_link_libraries(WindowsSearcher
        user32
        gdi32
        shell32
        ole32
        uuid
        comctl32)