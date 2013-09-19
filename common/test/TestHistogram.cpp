/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BOOST_TEST_MODULE TestHistogram
#include <boost/test/unit_test.hpp>
#include "TestConfig.h"
#include "../Histogram.h"

using namespace std;
using namespace mozilla::telemetry;

BOOST_AUTO_TEST_CASE(test_load)
{
  string fn(kDataPath + "cache/ad0ae007aa9e.json");
  try {
    Histogram h(fn);
    shared_ptr<HistogramDefinition> hd = h.GetDefinition("CYCLE_COLLECTOR");
    BOOST_REQUIRE(hd);
    int bc = hd->GetBucketCount();
    BOOST_REQUIRE_EQUAL(50, bc);
    int index = hd->GetBucketIndex(17);
    BOOST_REQUIRE_EQUAL(12, index);
    index = hd->GetBucketIndex(18);
    BOOST_REQUIRE_EQUAL(-1, index);
    shared_ptr<HistogramDefinition> nf = h.GetDefinition("NOT_FOUND");
    BOOST_REQUIRE(!nf);
  }
  catch (const exception& e) {
    BOOST_FAIL(e.what());
  }
}

BOOST_AUTO_TEST_CASE(test_missing_file)
{
  string fn(kDataPath + "missing.json");
  try {
    Histogram h(fn); 
    BOOST_FAIL("exception expected");
  }
  catch (const exception& e) {
    BOOST_REQUIRE_EQUAL(e.what(), "file open failed: " + fn);
  }
}

BOOST_AUTO_TEST_CASE(test_invalid_file)
{
  string fn(kDataPath + "invalid.json");
  try {
    Histogram h(fn); 
    BOOST_FAIL("exception expected");
  }
  catch (const exception& e) {
    BOOST_REQUIRE_EQUAL(e.what(), "json parse failed: Expect either an object" 
                        " or array at root");
  }
}

BOOST_AUTO_TEST_CASE(test_invalid_schema)
{
  string fn(kDataPath + "invalid_schema.json");
  try {
    Histogram h(fn); 
    BOOST_FAIL("exception expected");
  }
  catch (const exception& e) {
    BOOST_REQUIRE_EQUAL(e.what(), "histograms element must be an object");
  }
}

BOOST_AUTO_TEST_CASE(test_missing_kind)
{
  string fn(kDataPath + "missing_kind.json");
  try {
    Histogram h(fn); 
    BOOST_FAIL("exception expected");
  }
  catch (const exception& e) {
    BOOST_REQUIRE_EQUAL(e.what(), "Key: 'MY_HISTOGRAM' missing kind element");
  }
}

BOOST_AUTO_TEST_CASE(test_invalid_kind)
{
  string fn(kDataPath + "invalid_kind.json");
  try {
    Histogram h(fn); 
    BOOST_FAIL("exception expected");
  }
  catch (const exception& e) {
    BOOST_REQUIRE_EQUAL(e.what(), "Key: 'MY_HISTOGRAM' bad lexical cast: source"
                        " type value could not be interpreted as target");
  }
}