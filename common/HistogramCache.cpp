/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Histogram cache implementation @file

#include "HistogramCache.h"

using namespace std;
namespace fs = boost::filesystem;

namespace mozilla {
namespace telemetry {

////////////////////////////////////////////////////////////////////////////////
HistogramCache::HistogramCache(const fs::path &aCachePath) : 
mCachePath(aCachePath)
{
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<Histogram>
HistogramCache::LoadHistogram(const std::string &aRevisionKey)
{
  fs::path fn(mCachePath / (aRevisionKey + ".json"));
  if (!exists(fn)) return shared_ptr<Histogram>();

  try {
    shared_ptr<Histogram> h(new Histogram(fn));
    auto p = mCache.insert(make_pair(aRevisionKey, h));
    return p.first->second;
  }
  catch (exception &e) {
    // todo log error - bad histogram specification
  }
  return shared_ptr<Histogram>(); 
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<Histogram>
HistogramCache::FindHistogram(const std::string &aRevisionKey)
{
  shared_ptr<Histogram> h;

  auto it = mCache.find(aRevisionKey);
  if (it != mCache.end()) {
    h = it->second;
  } else {
    h = LoadHistogram(aRevisionKey);
  }
  return h;
}

////////////////////////////////////////////////////////////////////////////////
std::string 
HistogramCache::GetRevisionKey(const std::string &aRevisionURL)
{
  // http://hg.mozilla.org/releases/mozilla-release/rev/a55c55edf302
  size_t pos = aRevisionURL.find_last_of("/");
  if (pos != string::npos) {
    return aRevisionURL.substr(pos+1);
  }
  return aRevisionURL;
}

}
}
