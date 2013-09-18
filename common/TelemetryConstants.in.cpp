/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Prevent duplication of string constants between compilation units @file

#include "@CMAKE_CURRENT_SOURCE_DIR@/TelemetryConstants.h"

namespace mozilla {
namespace telemetry {

const unsigned kVersionMajor = @CPACK_PACKAGE_VERSION_MAJOR@;
const unsigned kVersionMinor = @CPACK_PACKAGE_VERSION_MINOR@;
const unsigned kVersionPatch = @CPACK_PACKAGE_VERSION_PATCH@;

const std::string kProgramName("@PROJECT_NAME@");
const std::string kProgramDescription("@CPACK_PACKAGE_DESCRIPTION_SUMMARY@");

// Record separator, path length, data length, timestamp (Unix epoch (ms))
//const int kTelemetryHeaderSize = sizeof(char) + sizeof(int32_t) +
//  sizeof(int32_t) + sizeof(int64_t);

// path length, data length, timestamp (Unix epoch (ms))
const size_t kTelemetryHeaderSize = sizeof(int32_t) + sizeof(int32_t) +
  sizeof(int64_t);
const size_t kMaxTelemetryPath = 10 * 1024;
const size_t kMaxTelemetryData = 200 * 1024;
const size_t kMaxTelemetryRecord = kTelemetryHeaderSize + kMaxTelemetryPath +
  kMaxTelemetryData;

const char kRecordSeparator = 0x1e;

const size_t kExtraBucketsSize = 5;
const char* kExtraBuckets[] = { "sum", "log_sum", "log_sum_squares",
  "sum_squares_lo", "sum_squares_hi", nullptr };

}
}
