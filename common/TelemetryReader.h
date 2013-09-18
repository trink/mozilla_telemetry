/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// Telemetry log reader. @file

#ifndef mozilla_telemetry_Telemetry_Reader_h
#define mozilla_telemetry_Telemetry_Reader_h

#include <cstdint>
#include <istream>
#include <rapidjson/document.h>

namespace mozilla {
namespace telemetry {

template<class T>
std::istream& read_value(std::istream& aInput, T& val)
{
  aInput.read((char*)&val, sizeof(val));
  return aInput;
}

struct TelemetryRecord
{
  uint64_t mTimestamp;
  std::string mPath;
  rapidjson::Document mDocument;
};

class TelemetryReader {
public:
  TelemetryReader(std::istream& aInput);
  ~TelemetryReader();

  bool Read(TelemetryRecord &aRec);

private:
  int Inflate();

  std::istream& mInput;

  uint32_t mPathLength;
  uint32_t mDataLength;
  uint32_t mInflateLength;

  size_t mPathSize;
  size_t mDataSize;
  size_t mInflateSize;

  char*   mPath;
  char*   mData;
  char*   mInflate;
};

}
}

#endif // mozilla_telemetry_Telemetry_Reader_h
