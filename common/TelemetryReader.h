/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// Telemetry log reader. @file

#ifndef mozilla_telemetry_Telemetry_Reader_h
#define mozilla_telemetry_Telemetry_Reader_h

#include <boost/utility.hpp>
#include <cstdint>
#include <istream>
#include <rapidjson/document.h>

namespace mozilla {
namespace telemetry {

struct TelemetryRecord
{
  uint64_t mTimestamp;
  std::string mPath;
  rapidjson::Document mDocument;
};

class TelemetryReader : boost::noncopyable
{
public:
  TelemetryReader(std::istream& aInput);
  ~TelemetryReader();
  
  /**
   * Reads the next available telemetry record from the stream.
   * 
   * @param aRec Record to populate.
   * 
   * @return bool True if a record was found, false if not.
   */
  bool Read(TelemetryRecord& aRec);

private:
  bool FindRecord(TelemetryRecord& aRec);
  bool ReadHeader(TelemetryRecord& aRec);
  bool ProcessRecord(TelemetryRecord& aRec);
  int Inflate();

  std::istream& mInput;

  uint16_t mPathLength;
  uint32_t mDataLength;
  uint32_t mInflateLength;

  size_t mPathSize;
  size_t mDataSize;
  size_t mInflateSize;

  size_t mBufferSize;
  size_t mScanPos;
  size_t mEndPos;

  char*   mPath;
  char*   mData;
  char*   mInflate;
  char*   mBuffer;

  bool  mReadHeader;
};

}
}

#endif // mozilla_telemetry_Telemetry_Reader_h
