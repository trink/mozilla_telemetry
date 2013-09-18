/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Histogram implementation @file

#include "Histogram.h"

#include <boost/scoped_array.hpp>
#include <boost/lexical_cast.hpp>
#include <exception>
#include <fstream>
#include <rapidjson/reader.h>

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
HistogramDefinition::GetBucketIndex(int aLowerBound) const
{
  auto it = mBuckets.find(aLowerBound);
  if (it == mBuckets.end()) {
    return -1;
  }
  return it->second;
}

////////////////////////////////////////////////////////////////////////////////
Histogram::Histogram(const boost::filesystem::path& fileName)
{
  ifstream ifs(fileName.c_str(), ifstream::binary);
  if (!ifs) {
    stringstream ss;
    ss << "file open failed: " << fileName.string();
    throw runtime_error(ss.str());
  }

  ifs.seekg(0, ifs.end);
  size_t len = ifs.tellg();
  ifs.seekg(0, ifs.beg);

  boost::scoped_array<char> buffer(new char[len + 1]);
  if (!ifs.read(buffer.get(), len)) {
    throw runtime_error("read failed");
  }
  ifs.close();

  buffer.get()[len] = 0;
  rapidjson::Document doc;
  if (doc.ParseInsitu<0>(buffer.get()).HasParseError()) {
    stringstream ss;
    ss << "json parse failed: " << doc.GetParseError();
    throw runtime_error(ss.str());
  }
  LoadDefinitions(doc);
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<HistogramDefinition>
Histogram::GetDefinition(const char* aName) const
{
  auto it = mDefinitions.find(aName);
  if (it != mDefinitions.end()) {
    return it->second;
  }
  return shared_ptr<HistogramDefinition>();
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
      shared_ptr<HistogramDefinition> def(new HistogramDefinition(it->value));
      mDefinitions.insert(make_pair(name, def));
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
