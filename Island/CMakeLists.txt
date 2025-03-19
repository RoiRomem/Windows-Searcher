cmake_minimum_required(VERSION 3.30)
project(Island)

set(CMAKE_CXX_STANDARD 26)

# Specify the sources for the project
add_executable(Island WIN32 main.cpp Island.h util.cpp widgetData.cpp)

# Enable asynchronous support
target_compile_options(Island PRIVATE /await)

# Link Windows API libraries for media session control and other necessary libraries
target_link_libraries(Island PRIVATE
        windowsapp          # Link to Windows App API
        ole32.lib           # Link to OLE32 for COM functionality
        user32.lib          # Link to User32 for basic windowing functionality
        mf.lib              # Media Foundation library (for media functionality)
        mfplat.lib          # Media Foundation platform library
        propsys.lib         # Library for property store and metadata
)
