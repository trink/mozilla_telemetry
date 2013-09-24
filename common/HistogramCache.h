/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @file 
Retrieves the requested histogram revision from cache.  If not cached checks for
and loads the histogram file from disk and adds it to the cache. 
*/

#ifndef mozilla_telemetry_Histogram_Cache_h
#define mozilla_telemetry_Histogram_Cache_h

#include "Histogram.h"

#include <boost/filesystem.hpp>
#include <map>
#include <memory>
#include <unordered_map>
#include <string>

namespace mozilla {
namespace telemetry {

class HistogramCache
{
public:
  HistogramCache(const std::string& aCacheURL);

  /**
   * Retrieves the requested histogram revision from cache.  If not cached it 
   * will attempt to load the file from the histogram server and add it to the 
   * cache. 
   * 
   * @param aRevision RevisionKey of the histogram file to load.
   * 
   * @return const Histogram* nullptr if load fails
   */
  std::shared_ptr<Histogram> FindHistogram(const std::string &aRevisionKey);

private:
  /**
   * Retrieves the requested histogram revision from the histogram server.
   * 
   * @param aRevisionKey Revision of the histogram file to load.
   * 
   * @return const Histogram* nullptr if load fails
   */
  std::shared_ptr<Histogram> LoadHistogram(const std::string &aRevisionKey);

  std::string mCacheHost;
  std::string mCachePort;

  /// Cache of histogram schema keyed by MD5
  std::unordered_map<std::string, std::shared_ptr<Histogram>> mCache;

  /// Cache of histogram schema keyed by revision
  std::map<std::string, std::shared_ptr<Histogram>> mRevisions;
};

}
}

#endif // mozilla_telemetry_Histogram_Cache_h
