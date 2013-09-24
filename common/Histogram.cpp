/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Histogram implementation @file

#include "Histogram.h"

#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <exception>
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/filestream.h>

using namespace std;
namespace mozilla {
namespace telemetry {

////////////////////////////////////////////////////////////////////////////////
HistogramDefinition::HistogramDefinition(const rapidjson::Value& aValue)
{
  const rapidjson::Value& k = aValue["kind"];
  if (!k.IsString()) {
    throw runtime_error("missing kind element");
  }
  mKind = boost::lexical_cast<int>(k.GetString());

  const rapidjson::Value& mn = aValue["min"];
  if (!mn.IsInt()) {
    throw runtime_error("missing min element");
  }
  mMin = mn.GetInt();

  const rapidjson::Value& mx = aValue["max"];
  if (!mx.IsInt()) {
    throw runtime_error("missing max element");
  }
  mMax = mx.GetInt();

  const rapidjson::Value& b = aValue["bucket_count"];
  if (!b.IsInt()) {
    throw runtime_error("missing bucket_count element");
  }
  mBucketCount = b.GetInt();

  const rapidjson::Value& a = aValue["buckets"];
  if (!a.IsArray()) {
    throw runtime_error("missing bucket array element");
  }
  int index = 0;
  for (rapidjson::Value::ConstValueIterator it = a.Begin(); it != a.End();
       ++it, ++index) {
    if (!it->IsInt()) {
      throw runtime_error("buckets array must contain integer elements");
    }
    mBuckets.insert(make_pair(it->GetInt(), index));
  }
  if (index != mBucketCount) {
    stringstream ss;
    ss << "buckets array should contain: " << mBucketCount << " elements;  "
      << index << " were specified";
    throw runtime_error(ss.str());
  }
}

////////////////////////////////////////////////////////////////////////////////
int
HistogramDefinition::GetBucketIndex(long aLowerBound) const
{
  auto it = mBuckets.find(aLowerBound);
  if (it == mBuckets.end()) {
    return -1;
  }
  return it->second;
}

////////////////////////////////////////////////////////////////////////////////
Histogram::Histogram(const std::string& aJSON)
{
  rapidjson::Document doc;
  if (doc.Parse<0>(aJSON.c_str()).HasParseError()) {
    stringstream ss;
    ss << "json parse failed: " << doc.GetParseError();
    throw runtime_error(ss.str());
  }
  LoadDefinitions(doc);
}

////////////////////////////////////////////////////////////////////////////////
Histogram::~Histogram()
{
  auto end = mDefinitions.end();
  for (auto it = mDefinitions.begin(); it != end; ){
    char* key = it->first;
    HistogramDefinition* hd = it->second;
    mDefinitions.erase(it++);
    delete[] key;
    delete hd;
  }
}

////////////////////////////////////////////////////////////////////////////////
const HistogramDefinition*
Histogram::GetDefinition(const char* aName) const
{
  auto it = mDefinitions.find(const_cast<char*>(aName));
  if (it != mDefinitions.end()) {
    return it->second;
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
/// Private Member Functions
////////////////////////////////////////////////////////////////////////////////
void
Histogram::LoadDefinitions(const rapidjson::Document& aDoc)
{
  const rapidjson::Value& histograms = aDoc["histograms"];
  if (!histograms.IsObject()) {
    throw runtime_error("histograms element must be an object");
  }
  for (rapidjson::Value::ConstMemberIterator it = histograms.MemberBegin();
       it != histograms.MemberEnd(); ++it) {
    const char* name = it->name.GetString();
    if (!it->value.IsObject()) {
      stringstream ss;
      ss << "Key: '" << name << "' is not a histogram definition";
      throw runtime_error(ss.str());
    }
    try {
      HistogramDefinition* hd = new HistogramDefinition(it->value);
      char* key = new char[strlen(name) + 1];
      strcpy(key, name);
      mDefinitions.insert(make_pair(key, hd));
    }
    catch (exception& e) {
      stringstream ss;
      ss <<  "Key: '" << name << "' " << e.what();
      throw runtime_error(ss.str());
    }
  }
}

}
}
