cmake_minimum_required(VERSION 3.15)

project(SPF_Viewer)

# This project is self contained.
# No dependency other than libc, libstdc++, libm and opengl,
# which are usually already provided by your operating system.


add_subdirectory(glfw-3.4)

# In order for -flto to take effect, it must be passed to both compiler and linker
target_compile_options(glfw PRIVATE -flto -Os)
target_link_options(glfw PRIVATE -flto)


include_directories(glfw-3.4/include)

file(GLOB SOURCES *.h *.c *.cpp)
add_executable(SPF_Viewer ${SOURCES})

target_compile_options(SPF_Viewer PRIVATE -flto -Os)
target_link_options(SPF_Viewer PRIVATE -s -flto)
if(WIN32)
    # Toolchain for Windows: llvm-mingw (https://github.com/mstorsjo/llvm-mingw)
    target_link_options(SPF_Viewer PRIVATE -mwindows)
endif()

target_link_libraries(SPF_Viewer glfw)
