# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include(ExternalProject)
set_property(DIRECTORY PROPERTY EP_BASE "${CMAKE_BINARY_DIR}/ep_base")
find_program(PATCH_EXECUTABLE patch)
if (NOT PATCH_EXECUTABLE)
   message(FATAL_ERROR "patch not found")
endif()

externalproject_add(
    rapidjson
    SVN_REPOSITORY http://rapidjson.googlecode.com/svn/trunk/
    PATCH_COMMAND ${PATCH_EXECUTABLE} -p1 < ${CMAKE_CURRENT_LIST_DIR}/rapidjson.patch
    BUILD_COMMAND ""
    CONFIGURE_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND "" # comment out to enable updates
)

set(RAPIDJSON_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/ep_base/Source/rapidjson/include")
include_directories(${RAPIDJSON_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${RAPIDJSON_INCLUDE_DIRS}")

