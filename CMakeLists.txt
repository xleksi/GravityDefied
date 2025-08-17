cmake_minimum_required(VERSION 4.1.0)
project(gravity-defied VERSION 0.1.0 LANGUAGES C CXX)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows")

find_package(raylib CONFIG REQUIRED)
add_executable(gravity-defied main.cpp)

target_link_libraries(gravity-defied PRIVATE raylib)
