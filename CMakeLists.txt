cmake_minimum_required(VERSION 3.22)
project(EvRGB_Combo_SDK)

# Read version information from header file early so all packaging steps have it
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/core/version.h" VERSION_FILE_CONTENT)

# Extract version numbers with sensible defaults
set(_evrgb_keys MAJOR MINOR PATCH)
set(_evrgb_defaults 1 0 0)

foreach(_key _default IN ZIP_LISTS _evrgb_keys _evrgb_defaults)
    if(VERSION_FILE_CONTENT MATCHES "EVRGB_VERSION_${_key} ([0-9]+)")
        set(EVRGB_VERSION_${_key} "${CMAKE_MATCH_1}")
    else()
        set(EVRGB_VERSION_${_key} "${_default}")
    endif()
endforeach()

set(EVRGB_VERSION_STRING "${EVRGB_VERSION_MAJOR}.${EVRGB_VERSION_MINOR}.${EVRGB_VERSION_PATCH}")

# Options
option(BUILD_TESTS "Build test programs" OFF)
option(BUILD_SAMPLES "Build sample programs" ON)

# set c++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories for all targets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 设置默认的构建类型为 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Include GNUInstallDirs for standard installation directories
include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(external/loguru)
find_package(OpenCV REQUIRED)

# Set include directories for the entire project
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    external/loguru
)

add_subdirectory(src)

# Conditionally add test directory
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# Conditionally add sample directory
if(BUILD_SAMPLES)
    add_subdirectory(sample)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

message(STATUS "EvRGB Combo SDK Version: ${EVRGB_VERSION_STRING}")

# Install header files
install(DIRECTORY include/ 
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")

# install(FILES external/loguru/loguru.hpp DESTINATION include)

# Configure and install config files for find_package support
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/evrgb_combo-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/evrgb_combo-config.cmake"
    INSTALL_DESTINATION lib/cmake/evrgb_combo
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/evrgb_combo-config-version.cmake"
    VERSION "${EVRGB_VERSION_STRING}"
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/evrgb_combo-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/evrgb_combo-config-version.cmake"
    DESTINATION lib/cmake/evrgb_combo
)

# Install custom Find modules
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindMVS.cmake"
    DESTINATION lib/cmake/evrgb_combo
)

# CPack Configuration
set(CPACK_PACKAGE_NAME "evrgb_combo_sdk")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Dvsense Combo SDK for RGB and DVS cameras")
set(CPACK_PACKAGE_VERSION "${EVRGB_VERSION_STRING}")
set(CPACK_PACKAGE_VERSION_MAJOR "${EVRGB_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${EVRGB_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${EVRGB_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "evrgb_combo_sdk")
set(CPACK_PACKAGE_CONTACT "tonywangziteng")

# License file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()

# Generators
set(CPACK_GENERATOR "TGZ;DEB")
set(CPACK_SOURCE_GENERATOR "TGZ")

# Debian specific
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "tonywangziteng")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)

