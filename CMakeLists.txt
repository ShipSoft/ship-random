# SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
#
# SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required(VERSION 3.24)
project(
    SHiPRandom
    VERSION 0.4.0
    DESCRIPTION "Random number helpers shared by the SHiP software"
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(SHIPRANDOM_BUILD_TESTS "Build the tests" ${PROJECT_IS_TOP_LEVEL})

# Random123 is header-only and ships no CMake config. Only the library's own
# source includes it, so consumers do not need it.
find_path(Random123_INCLUDE_DIR Random123/philox.h REQUIRED)

add_library(SHiPRandom src/philox_rng.cpp)
add_library(SHiP::Random ALIAS SHiPRandom)
# 0.x: a minor release may change the sequences or the interface, so it gets
# its own soname.
set_target_properties(
    SHiPRandom
    PROPERTIES
        EXPORT_NAME Random
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)
target_compile_features(SHiPRandom PUBLIC cxx_std_20)
target_include_directories(
    SHiPRandom
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(SHiPRandom SYSTEM PRIVATE ${Random123_INCLUDE_DIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS SHiPRandom EXPORT SHiPRandomTargets)
install(
    EXPORT SHiPRandomTargets
    NAMESPACE SHiP::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SHiPRandom
)
configure_package_config_file(
    cmake/SHiPRandomConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SHiPRandomConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SHiPRandom
)
# 0.x: a minor release may change the sequences or the interface.
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/SHiPRandomConfigVersion.cmake
    COMPATIBILITY SameMinorVersion
)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/SHiPRandomConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/SHiPRandomConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SHiPRandom
)

if(SHIPRANDOM_BUILD_TESTS)
    enable_testing()
    add_executable(philox_rng_test tests/philox_rng_test.cpp)
    target_link_libraries(philox_rng_test PRIVATE SHiP::Random)
    target_compile_features(philox_rng_test PRIVATE cxx_std_23)
    add_test(NAME philox_rng COMMAND philox_rng_test)

    add_executable(stream_pin_test tests/stream_pin_test.cpp)
    target_link_libraries(stream_pin_test PRIVATE SHiP::Random)
    target_compile_features(stream_pin_test PRIVATE cxx_std_23)
    add_test(NAME stream_pins COMMAND stream_pin_test)
endif()
