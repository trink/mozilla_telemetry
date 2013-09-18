/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Histogram converter implementation @file

#include "HistogramConverter.h"
#include "TelemetryConstants.h"

#include <boost/lexical_cast.hpp>
#include <memory>
#include <vector>

using namespace std;

namespace mozilla {
namespace telemetry {

bool RewriteValues(shared_ptr<HistogramDefinition> aDef,
                   const rapidjson::Value& aData,
                   vector<int>& aRewrite);

bool RewriteHistogram(shared_ptr<Histogram>& aHist, rapidjson::Value& aValue);


////////////////////////////////////////////////////////////////////////////////
bool ConvertHistogramData(HistogramCache& aCache, rapidjson::Document& aDoc)
{
  const rapidjson::Value& info = aDoc["info"];
  if (!info.IsObject()) {
    // todo log error - missing info object
    return false;
  }

  const rapidjson::Value& revision = info["revision"];
  if (!revision.IsString()) {
    // todo log error - missing info.revision
    return false;
  }

  rapidjson::Value& histograms = aDoc["histograms"];
  if (!histograms.IsObject()) {
    // todo log error - missing histograms object
    return false;
  }

  rapidjson::Value& ver = aDoc["ver"];
  if (!ver.IsInt() || ver.GetInt() != 1) {
    // todo log error - missing version
    return false;
  }

  bool result = true;
  switch (ver.GetInt()) {
  case 1:
    {
      string rev = aCache.GetRevisionKey(revision.GetString());
      shared_ptr<Histogram> hist = aCache.FindHistogram(rev);
      if (hist) {
        result = RewriteHistogram(hist, histograms); 
        if (result) {
          ver.SetInt(2);
        } else {
          ver.SetInt(-1);
        }
      } else {
        // todo log error - histogram not found
      }
    }
    break;
  case 2: // already converted
    break;
  default:
    // todo log error - invalid version
    break;
  }

  return result;
}

////////////////////////////////////////////////////////////////////////////////
bool RewriteValues(std::shared_ptr<HistogramDefinition> aDef,
                   const rapidjson::Value& aData,
                   std::vector<double>& aRewrite)
{
  const rapidjson::Value& values = aData["values"];
  if (!values.IsObject()) {
    // todo log error - value object not found
    return false;
  }
  for (rapidjson::Value::ConstMemberIterator it = values.MemberBegin();
       it != values.MemberEnd(); ++it) {
    if (!it->value.IsInt()) {
      // todo log error - invalid value object
      return false;
    }
    int lb = 0;
    try {
      lb = boost::lexical_cast<int>(it->name.GetString());
    }
    catch (boost::bad_lexical_cast&) {
      // todo log error - non integer bucket lower bound
      return false;
    }
    int i = it->value.GetInt();
    int index = aDef->GetBucketIndex(lb);
    if (index == -1) {
      // todo log error - invalid bucket lower bound
      return false;
    }
    aRewrite[index] = i;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool RewriteHistogram(shared_ptr<Histogram>& aHist, rapidjson::Value& aValue)
{
  rapidjson::Document doc;
  rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
  std::vector<double> rewrite;
  bool result = true;

  for (rapidjson::Value::MemberIterator it = aValue.MemberBegin(); result &&
       it != aValue.MemberEnd(); ++it) {
    if (it->value.IsObject()) {
      const char* name = reinterpret_cast<const char*>(it->name.GetString());
      std::shared_ptr<HistogramDefinition> hd = aHist->GetDefinition(name);
      if (!hd) {
        if (strncmp(name, "STARTUP_", 8)) {
          hd = aHist->GetDefinition(name + 8);
          if (hd) {
            it->name.SetString(name + 8);
            // chop off leading "STARTUP_" per
            // http://mxr.mozilla.org/mozilla-central/source/toolkit/components/telemetry/TelemetryPing.js#532
          }
        }
      }
      if (hd) {
        int bucketCount = hd->GetBucketCount();
        int arraySize = bucketCount + kExtraBucketsSize;
        rewrite.clear();
        rewrite.resize(arraySize);
        result = RewriteValues(hd, it->value, rewrite);
        if (result) {
          // append the summary data at the end of the array
          for (int x = 0; kExtraBuckets[x] != nullptr; ++x) {
            const rapidjson::Value& v = it->value[kExtraBuckets[x]];
            if (v.IsNumber()) {
              rewrite[bucketCount + x] = v.GetDouble();
            } else {
              rewrite[bucketCount + x] = -1;
            }
          }
          // rewrite the JSON histogram data
          it->value.SetArray();
          it->value.Reserve(arraySize, alloc);
          auto end = rewrite.end();
          for (auto vit = rewrite.begin(); vit != end; ++vit){
            it->value.PushBack(*vit, alloc);
          }
        }
      } else {
        // todo log warning - histogram definitition lookup failed
      }
    } else {
      // todo log warning - not a histogram object
    }
  }
  return result;
}

}
}
