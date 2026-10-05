# SPDX-FileCopyrightText: 2026 CERN for the benefit of the SHiP Collaboration
#
# SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required(VERSION 3.24)
project(
    SHiPRandom
    VERSION 0.1.0
    DESCRIPTION "Random number helpers shared by the SHiP software"
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(SHIPRANDOM_BUILD_TESTS "Build the tests" ${PROJECT_IS_TOP_LEVEL})

# Random123 is header-only and ships no CMake config.
find_path(Random123_INCLUDE_DIR Random123/philox.h REQUIRED)

add_library(SHiPRandom INTERFACE)
add_library(SHiP::Random ALIAS SHiPRandom)
set_target_properties(SHiPRandom PROPERTIES EXPORT_NAME Random)
target_compile_features(SHiPRandom INTERFACE cxx_std_20)
target_include_directories(
    SHiPRandom
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
# The installed config finds Random123 again on the consumer's side, so the
# build machine's path is not baked into the exported target.
target_include_directories(
    SHiPRandom
    SYSTEM
    INTERFACE $<BUILD_INTERFACE:${Random123_INCLUDE_DIR}>
)

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
    ARCH_INDEPENDENT
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
endif()
