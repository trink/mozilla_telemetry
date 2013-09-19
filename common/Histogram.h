/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @file 
Accessor and utility functions functions for the Histogram.json data structure. 
 */

#ifndef mozilla_telemetry_Histogram_h
#define mozilla_telemetry_Histogram_h

#include <boost/filesystem.hpp>
#include <memory>
#include <rapidjson/document.h>
#include <unordered_map>
#include <vector>

namespace mozilla {
namespace telemetry {

/** 
 * Stores a specific histogram definition within a histogram file
 * 
 */
class HistogramDefinition
{
public:

  HistogramDefinition(const rapidjson::Value& aValue);

  /**
   * Returns the index of the associated bucket based on the bucket's lower 
   * bound. 
   * 
   * @param aLowerBound The lower bound of the data stored in this bucket
   * 
   * @return int The bucket index or -1 if the lower bound is invalid.
   */
  int GetBucketIndex(int aLowerBound) const;

  /**
   * Returns the number of counter buckets in the definition.
   * 
   * @return int Number of buckets.
   */
  int GetBucketCount() const;

private:
  int mKind;
  int mMin;
  int mMax;
  int mBucketCount;
  std::unordered_map<int, int> mBuckets;
};

inline int HistogramDefinition::GetBucketCount() const
{
  return mBucketCount;
}

/** 
 * Stores the set of histogram definitions within a histogram file.
 * 
 */
class Histogram
{
public:
  /**
   * Loads the specified Histogram.json file from disk into memory
   * 
   * @param fileName Fully qualified name of histogram file.
   * 
   */
  Histogram(const boost::filesystem::path& fileName);


  /**
   * Retrieve a specific histogram definition by name.
   * 
   * @param aName Histogram name.
   * 
   * @return HistogramDefinition Histogram definition or nullptr if the
   * definition is not found.
   */
  std::shared_ptr<HistogramDefinition> GetDefinition(const char* aName) const;

private:

  /**
   * Loads the histogram definitions/verifies the schema
   * 
   * @param aValue "histograms" object from the JSON document.
   * 
   */
  void LoadDefinitions(const rapidjson::Document& aDoc);

  std::unordered_map<std::string, std::shared_ptr<HistogramDefinition>>
                     mDefinitions;
};

}
}

#endif // mozilla_telemetry_Histogram_h