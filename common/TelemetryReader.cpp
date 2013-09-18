/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Telemetry reader implementation @file

#include "TelemetryConstants.h"
#include "TelemetryReader.h"

#include <cstring>
#include <iostream>
#include <zlib.h>

using namespace std;

namespace mozilla {
namespace telemetry {

///////////////////////////////////////////////////////////////////////////////
TelemetryReader::TelemetryReader(std::istream& aInput) :
  mInput(aInput),

  mPathLength(0),
  mDataLength(0),
  mInflateLength(0),

  mPathSize(kMaxTelemetryPath),
  mDataSize(kMaxTelemetryData),
  mInflateSize(kMaxTelemetryRecord),

  mPath(nullptr),
  mData(nullptr),
  mInflate(nullptr)
{
  mPath = new char[mPathSize + 1];
  mData = new char[mDataSize + 1];
  mInflate = new char[mInflateSize];
}

////////////////////////////////////////////////////////////////////////////////
TelemetryReader::~TelemetryReader()
{
  delete[] mPath;
  delete[] mData;
  delete[] mInflate;
}

////////////////////////////////////////////////////////////////////////////////
bool TelemetryReader::Read(TelemetryRecord& aRec)
{
  mPathLength = 0;
  mDataLength = 0;
  mInflateLength = 0;

  if (mInput) {
    read_value(mInput, mPathLength); // todo support conversion to big endian?
    read_value(mInput, mDataLength);
    read_value(mInput, aRec.mTimestamp);
    if (mPathLength > mPathSize) {
      // todo seek for next valid record, but for now stop processing
      return false;
    }
    mInput.read(mPath, mPathLength);
    aRec.mPath = string(mPath, mPathLength);

    if (mDataLength > mDataSize) {
      // todo seek for next valid record, but for now stop processing
      return false;
    }
    mInput.read(mData, mDataLength);

    if (mDataLength > 2 && mData[0] == 0x1f
        && static_cast<unsigned char>(mData[1]) == 0x8b) {
      int ret = Inflate(); // this increases processing time by 70x
      if (ret != Z_OK) {
        // todo log error - ungzip failed, the document will be empty
      } else {
        if (mInflateLength < mInflateSize) {
          mInflate[mInflateLength] = 0;
        } else {
          size_t required = mInflateLength + 1; // make room for the null
          char* tmp = new char[required];
          if (tmp) {
            memcpy(tmp, mInflate, mInflateLength);
            delete[] mInflate;
            mInflate = tmp;
            mInflateSize = required;
            mInflate[mInflateLength] = 0;
          }
        }
        aRec.mDocument.Parse<0>(mInflate); // cannot use the destructive parse
        // unless the inflate buffer is stored in the TelemetryRecord
      }
    } else {
      mData[mDataLength] = 0;
      aRec.mDocument.Parse<0>(mData);
    }
  }
  return mInput.good();
}

////////////////////////////////////////////////////////////////////////////////
int TelemetryReader::Inflate()
{
  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = mDataLength;
  strm.next_in = reinterpret_cast<unsigned char*>(mData);
  strm.avail_out = mInflateSize;
  strm.next_out = reinterpret_cast<unsigned char*>(mInflate);

  int ret = inflateInit2(&strm, 16 + MAX_WBITS);
  if (ret != Z_OK) {
    return ret;
  }

  do {
    if (ret == Z_BUF_ERROR) {
      size_t required = mInflateSize * 2;
      if (required > kMaxTelemetryRecord) {
        if (mInflateSize < kMaxTelemetryRecord) {
          required = kMaxTelemetryRecord;
        } else {
          break;
        }
      }
      char* tmp = new char[required];
      if (tmp) {
        memcpy(tmp, mInflate, mInflateLength);
        delete[] mInflate;
        mInflate = tmp;
        mInflateSize = required;
        strm.avail_out = mInflateSize - mInflateLength;
        strm.next_out = reinterpret_cast<unsigned char*>(mInflate +
                                                         mInflateLength);
      } else {
        break;
      }
    }
    ret = inflate(&strm, Z_FINISH);
    mInflateLength = mInflateSize - strm.avail_out;
  }
  while (ret == Z_BUF_ERROR);

  inflateEnd(&strm);
  return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

}
}
