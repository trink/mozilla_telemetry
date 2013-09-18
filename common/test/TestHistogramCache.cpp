/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BOOST_TEST_MODULE TestHistogramCache
#include <boost/test/unit_test.hpp>
#include "TestConfig.h"
#include "../HistogramCache.h"

using namespace std;
using namespace mozilla::telemetry;


BOOST_AUTO_TEST_CASE(test_get_revision)
{
  string r25("http://hg.mozilla.org/releases/mozilla-release/rev/ad0ae007aa9e");
  string r24("8d3810543edc");
  BOOST_REQUIRE_EQUAL("ad0ae007aa9e", HistogramCache::GetRevisionKey(r25));
  BOOST_REQUIRE_EQUAL(r24, HistogramCache::GetRevisionKey(r24));
}

BOOST_AUTO_TEST_CASE(test_load)
{
  string cachePath(kDataPath + "cache");
  string r25("ad0ae007aa9e");
  string r24("8d3810543edc");

  HistogramCache cache(cachePath);
  auto h = cache.LoadHistogram(r25);
  BOOST_REQUIRE(h);
  auto h1 = cache.FindHistogram(r25);
  BOOST_REQUIRE_EQUAL(h, h1);
}

BOOST_AUTO_TEST_CASE(test_cache_miss)
{
  string cachePath(kDataPath + "cache");
  string r25("ad0ae007aa9e");

  HistogramCache cache(cachePath);
  auto h= cache.FindHistogram(r25);
  BOOST_REQUIRE(h);
}

BOOST_AUTO_TEST_CASE(test_missing_revision)
{
  string cachePath(kDataPath + "cache");
  HistogramCache cache(cachePath);
  auto h = cache.LoadHistogram("missing");
  BOOST_REQUIRE(!h);
}
