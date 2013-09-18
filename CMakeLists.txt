# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(telemetry)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Telemetry")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 1)
set(CPACK_PACKAGE_VERSION_PATCH 0)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(mozsvc)
include(externals)

find_package(ZLIB REQUIRED)
find_package(Boost 1.51.0 REQUIRED 
filesystem  
system 
unit_test_framework)

include_directories(${Boost_INCLUDE_DIRS} "${CMAKE_SOURCE_DIR}/common")

add_executable(convert convert.cpp)
target_link_libraries(convert telemetry)

add_subdirectory(common)

install(TARGETS convert DESTINATION bin)
