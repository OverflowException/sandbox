cmake_minimum_required(VERSION 3.0.0)
project(sandbox VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#set(CMAKE_BUILD_TYPE "Release")

add_subdirectory(3rdparty)

add_subdirectory(src)
# add_executable(sandbox main.cpp)

#target_link_libraries(sandbox PUBLIC bgfx)