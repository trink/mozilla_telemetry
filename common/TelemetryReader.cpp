/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Telemetry reader implementation @file

#include "TelemetryConstants.h"
#include "TelemetryReader.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <zlib.h>

#include <iostream>
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

  mBufferSize(kMaxTelemetryRecord),
  mScanPos(0),
  mEndPos(0),

  mPath(nullptr),
  mData(nullptr),
  mInflate(nullptr),
  
  mBuffer(nullptr),

  mReadHeader(false)
{
  mBuffer = new char[kMaxTelemetryRecord + 1];
  mInflate = new char[mInflateSize];
}

////////////////////////////////////////////////////////////////////////////////
TelemetryReader::~TelemetryReader()
{
  delete[] mBuffer;
  delete[] mInflate;
}

////////////////////////////////////////////////////////////////////////////////
bool TelemetryReader::Read(TelemetryRecord& aRec)
{
  size_t n = 0;
  while (mInput) {
    if (FindRecord(aRec)) {
      mReadHeader = false;
      if (ProcessRecord(aRec)) {
        aRec.mPath = string(mPath, mPathLength);
        return true;
      }
      continue;  // bad record try again
    } else if (mScanPos != mEndPos) { // realign buffer
      memmove(mBuffer, mBuffer + mScanPos, mEndPos - mScanPos);
      mEndPos = mEndPos - mScanPos;
      mScanPos = 0;
    }
    mInput.read(mBuffer + mEndPos, mBufferSize - mEndPos);
    n = mInput.gcount();
    mEndPos += n;
    if (mInput.eof()) mInput.clear();
    if (n == 0) break;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
/// Private Members
////////////////////////////////////////////////////////////////////////////////
bool TelemetryReader::FindRecord(TelemetryRecord& aRec)
{
  char* pos = find(mBuffer + mScanPos, mBuffer + mEndPos, kRecordSeparator);
  if (pos != mBuffer + mEndPos) {
    mScanPos = pos - mBuffer;
    size_t headerEnd = mScanPos + kTelemetryHeaderSize;
    if (headerEnd > mEndPos) return false; // need more data

    mReadHeader = ReadHeader(aRec);
    if (mReadHeader) {
      size_t recordEnd = mScanPos + kTelemetryHeaderSize + mPathLength + mDataLength;
      if (recordEnd > mEndPos) return false; // need more data
      mPath = mBuffer + mScanPos + kTelemetryHeaderSize;
      mData = mBuffer + mScanPos + kTelemetryHeaderSize + mPathLength;
      mScanPos = recordEnd;
      return true;
    } else {
      ++mScanPos;
      return FindRecord(aRec);
    }
  } else {
    mScanPos = mEndPos = 0;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool TelemetryReader::ReadHeader(TelemetryRecord& aRec)
{
  if (mReadHeader) return true;

  // todo support big endian?
  size_t pos = mScanPos + 1;
  memcpy(&mPathLength, mBuffer + pos, sizeof(mPathLength));
  if (mPathLength > kMaxTelemetryPath) return false;
  pos += sizeof(mPathLength);

  memcpy(&mDataLength, mBuffer + pos, sizeof(mDataLength));
  if (mDataLength > kMaxTelemetryData) return false;
  pos += sizeof(mDataLength);

  memcpy(&aRec.mTimestamp, mBuffer + pos, sizeof(aRec.mTimestamp));

  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool TelemetryReader::ProcessRecord(TelemetryRecord& aRec)
{
  if (mDataLength > 2 && mData[0] == 0x1f
      && static_cast<unsigned char>(mData[1]) == 0x8b) {
    int ret = Inflate();
    if (ret != Z_OK) {
      // todo log error - ungzip failed
      return false;
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
                                         // unless the inflate buffer is stored
                                         // in the TelemetryRecord
    }
  } else {
    char ch = mData[mDataLength]; // grab the byte following the data, the
                                  // underlying mBuffer owns it
    mData[mDataLength] = 0; // temporarily write a null into the buffer
    aRec.mDocument.Parse<0>(mData);
    mData[mDataLength] = ch; // restore the buffer
  }
  if (aRec.mDocument.HasParseError()) {
    // todo log error - aRec.mDocument.GetParseError()
    return false;
  }
  return true;
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
