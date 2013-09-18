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
#include <memory>
#include <unordered_map>
#include <string>

namespace mozilla {
namespace telemetry {

class HistogramCache
{
public:
  HistogramCache(const boost::filesystem::path &aCachePath);

  /**
   * Retrieves the requested histogram revision from disk.
   * 
   * @param aRevisionKey Revision of the histogram file to load.
   * 
   * @return const Histogram* nullptr if load fails
   */
  std::shared_ptr<Histogram> LoadHistogram(const std::string &aRevisionKey);

  /**
   * Retrieves the requested histogram revision from cache.  If not cached it 
   * will attempt to load the file from disk and add it to the cache. 
   * 
   * @param aRevision RevisionKey of the histogram file to load.
   * 
   * @return const Histogram* nullptr if load fails
   */
  std::shared_ptr<Histogram> FindHistogram(const std::string &aRevisionKey);

  /**
   * Extracts the revision identifier from the URL
   * 
   * @param aRevisionURL info.revision URL from the histogram data
   * 
   * @return std::string Revision identifer key used by the cache.
   */
  static std::string GetRevisionKey(const std::string &aRevisionURL);

private:

  boost::filesystem::path mCachePath;
  std::unordered_map<std::string, std::shared_ptr<Histogram>> mCache;
};

}
}

#endif // mozilla_telemetry_Histogram_Cache_h
