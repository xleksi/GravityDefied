cmake_minimum_required(VERSION 4.1.0)
project(gravity-defied VERSION 0.1.0 LANGUAGES C CXX)

# Tell CMake where vcpkg installed libraries
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows")

# Find raylib
find_package(raylib CONFIG REQUIRED)

# Grab all cpp files in the folder
file(GLOB SOURCES "*.cpp")

# Build executable
add_executable(gravity-defied ${SOURCES})

add_custom_command(TARGET gravity-defied POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:gravity-defied>/assets)

# Link raylib
target_link_libraries(gravity-defied PRIVATE raylib)