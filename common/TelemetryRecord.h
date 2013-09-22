/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// Telemetry record. @file

#ifndef mozilla_telemetry_Telemetry_Record_h
#define mozilla_telemetry_Telemetry_Record_h

#include "TelemetryConstants.h"

#include <boost/utility.hpp>
#include <cstdint>
#include <rapidjson/document.h>

namespace mozilla {
namespace telemetry {

template<class T>
std::istream& read_value(std::istream& aInput, T& val)
{
  aInput.read((char*)&val, sizeof(val));
  return aInput;
}

class TelemetryRecord : boost::noncopyable
{
public:
  TelemetryRecord();
  ~TelemetryRecord();

  bool Read(std::istream& aInput);

  const char* GetPath();
  uint64_t GetTimestamp();
  rapidjson::Document& GetDocument();

private:
  bool FindRecord(std::istream& aInput);
  bool ReadHeader(std::istream& aInput);
  bool ProcessRecord();
  int Inflate();

  rapidjson::Document mDocument;

  uint16_t  mPathLength;
  size_t    mPathSize;
  char*     mPath;

  uint32_t  mDataLength;
  size_t    mDataSize;
  char*     mData;

  uint64_t  mTimestamp;

  uint32_t  mInflateLength;
  size_t    mInflateSize;
  char*     mInflate;

};

}
}

#endif // mozilla_telemetry_Telemetry_Record_h
