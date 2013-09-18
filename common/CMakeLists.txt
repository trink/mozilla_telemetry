# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(TELEMETRY_SRC
TelemetryConstants.cpp 
Histogram.cpp 
HistogramCache.cpp
HistogramConverter.cpp 
TelemetryReader.cpp 
TelemetrySchema.cpp)

add_library(telemetry STATIC ${TELEMETRY_SRC})
add_dependencies(telemetry rapidjson-0_11)
target_link_libraries(telemetry ${Boost_LIBRARIES} ${ZLIB_LIBRARIES})

configure_file(TelemetryConstants.in.cpp ${CMAKE_CURRENT_BINARY_DIR}/TelemetryConstants.cpp)

add_subdirectory(test)
