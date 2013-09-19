/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BOOST_TEST_MODULE TestTelemetryRecord
#include <boost/test/unit_test.hpp>
#include "TestConfig.h"
#include "../HistogramCache.h"
#include "../HistogramConverter.h"
#include "../TelemetryReader.h"

#include <string>
#include <fstream>
#include <sstream>

#include <rapidjson/filestream.h>
#include <rapidjson/writer.h>

#include <iostream>

using namespace std;
using namespace mozilla::telemetry;


static const string rec("\x1e\x04\x00\x07\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00" "abcd{\"a\":8}", 26);
BOOST_AUTO_TEST_CASE(test_read)
{
  string data(rec + rec);
  istringstream iss(data);
  TelemetryReader reader(iss);
  TelemetryRecord tr;
  for (int i = 0; i < 2; ++i) {
    BOOST_REQUIRE_EQUAL(true, reader.Read(tr)); 
    BOOST_REQUIRE_EQUAL(1, tr.mTimestamp);
    BOOST_REQUIRE_EQUAL("abcd", tr.mPath);
    BOOST_REQUIRE_EQUAL(8, tr.mDocument["a"].GetInt());
  }
}

BOOST_AUTO_TEST_CASE(test_partial)
{
  string data(rec + "\x1e");
  istringstream iss(data);
  TelemetryReader reader(iss);
  TelemetryRecord tr;
  BOOST_REQUIRE_EQUAL(true, reader.Read(tr)); 
  BOOST_REQUIRE_EQUAL(1, tr.mTimestamp);
  BOOST_REQUIRE_EQUAL("abcd", tr.mPath);
  BOOST_REQUIRE_EQUAL(8, tr.mDocument["a"].GetInt());

  BOOST_REQUIRE_EQUAL(false, reader.Read(tr));

  iss.str(rec.substr(1));
  BOOST_REQUIRE_EQUAL(true, reader.Read(tr)); 
  BOOST_REQUIRE_EQUAL(1, tr.mTimestamp);
  BOOST_REQUIRE_EQUAL("abcd", tr.mPath);
  BOOST_REQUIRE_EQUAL(8, tr.mDocument["a"].GetInt());
}

BOOST_AUTO_TEST_CASE(test_exceed_pathlength)
{
  string data(rec + string("\x1e\xff\xff\x07\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00", 15) + rec);
  istringstream iss(data);
  TelemetryReader reader(iss);
  TelemetryRecord tr;
  for (int i = 0; i < 2; ++i) {
    BOOST_REQUIRE_EQUAL(true, reader.Read(tr)); 
    BOOST_REQUIRE_EQUAL(1, tr.mTimestamp);
    BOOST_REQUIRE_EQUAL("abcd", tr.mPath);
    BOOST_REQUIRE_EQUAL(8, tr.mDocument["a"].GetInt());
  }
}

BOOST_AUTO_TEST_CASE(test_short_pathlength)
{
  string bad_rec("\x1e\x02\x00\x07\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00" "abcd{\"a\":8}", 26);
  string data(bad_rec + rec);
  istringstream iss(data);
  TelemetryReader reader(iss);
  TelemetryRecord tr;

  BOOST_REQUIRE_EQUAL(true, reader.Read(tr)); 
  BOOST_REQUIRE_EQUAL(1, tr.mTimestamp);
  BOOST_REQUIRE_EQUAL("abcd", tr.mPath);
  BOOST_REQUIRE_EQUAL(8, tr.mDocument["a"].GetInt());

  BOOST_REQUIRE_EQUAL(false, reader.Read(tr));
}

BOOST_AUTO_TEST_CASE(test_convert)
{
  ifstream file(kDataPath + "telemetry1.log", ios_base::binary);
  TelemetryReader reader(file);
  TelemetryRecord tr;
  BOOST_REQUIRE_EQUAL(true, reader.Read(tr));

  HistogramCache cache(kDataPath + "cache");
  BOOST_REQUIRE_EQUAL(true, ConvertHistogramData(cache, tr.mDocument));
}