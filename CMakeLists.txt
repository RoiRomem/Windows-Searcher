cmake_minimum_required(VERSION 3.20)
project(WindowsSearcher)

set(CMAKE_CXX_STANDARD 20) # Changed to C++20
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(WindowsSearcher WIN32 main.cpp optioneer.cpp
        extraCommands.cpp)

target_compile_definitions(WindowsSearcher PRIVATE
        WIN32
)

target_link_libraries(WindowsSearcher
        kernel32
        user32
        gdi32
        winspool
        shell32
        ole32
        oleaut32
        uuid
        comdlg32
        advapi32
)
