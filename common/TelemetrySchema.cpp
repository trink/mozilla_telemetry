/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Telemetry schema implementation @file

#include "TelemetrySchema.h"

#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <fstream>
#include <rapidjson/document.h>
#include <sstream>

using namespace std;
namespace mozilla {
namespace telemetry {

////////////////////////////////////////////////////////////////////////////////
TelemetrySchema::TelemetryDimension::TelemetryDimension(const rapidjson::Value& aValue)
{
  namespace bx = boost::xpressive;
  const rapidjson::Value& fn = aValue["field_name"];
  if (!fn.IsString()) {
    throw runtime_error("missing field_name element");
  }
  mName = fn.GetString();

  const rapidjson::Value& av = aValue["allowed_values"];
  switch (av.GetType()) {
  case rapidjson::kStringType:
    mType = kValue;
    mValue = av.GetString();
    break;
  case rapidjson::kArrayType:
    mType = kSet;
    for (rapidjson::Value::ConstValueIterator it = av.Begin(); it != av.End();
         ++it) {
      if (!it->IsString()) {
        throw runtime_error("allowed_values must be strings");
      }
      mSet.insert(it->GetString());
    }
    break;
  case rapidjson::kObjectType:
    {
      mType = kRange;
      const rapidjson::Value& mn = av["min"];
      if (!mn.IsNumber()) {
        throw runtime_error("allowed_values range is missing min element");
      }
      const rapidjson::Value& mx = av["max"];
      if (!mx.IsNumber()) {
        throw runtime_error("allowed_values range is missing max element");
      }
      mRange.first = mn.GetDouble();
      mRange.second = mx.GetDouble();
    }
    break;
  default:
    throw runtime_error("invalid allowed_values element");
    break;
  }
}

////////////////////////////////////////////////////////////////////////////////
TelemetrySchema::TelemetrySchema(const boost::filesystem::path& fileName)
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
  if (doc.Parse<0>(buffer.get()).HasParseError()) {
    stringstream ss;
    ss << "json parse failed: " << doc.GetParseError();
    throw runtime_error(ss.str());
  }

  const rapidjson::Value& version = doc["version"];
  if (!version.IsInt()) {
    throw runtime_error("version element is missing");
  }
  mVersion = version.GetInt();
  LoadDimensions(doc);
}

////////////////////////////////////////////////////////////////////////////////
boost::filesystem::path
TelemetrySchema::GetDimensionPath(const rapidjson::Value& aInfo)
{
  static const string kOther("other");
  boost::filesystem::path p;
  auto end = mDimensions.end();
  for (auto it = mDimensions.begin(); it != end; ++it){
    const rapidjson::Value& v = aInfo[(*it)->mName.c_str()];
    if (v.IsString()) {
      string dim = v.GetString();
      switch ((*it)->mType) {
      case TelemetryDimension::kValue:
        if ((*it)->mValue == "*" || (*it)->mValue == dim) {
          p /= SafePath(dim);
        } else {
          p /= kOther;
        }
        break;
      case TelemetryDimension::kSet:
        if ((*it)->mSet.find(dim) != (*it)->mSet.end()) {
          p /= SafePath(dim);
        } else {
          p /= kOther;
        }
        break;
      default:
        // todo log error - range comparison not allowed on a string
        break;
      }
    } else if (v.IsNumber()) {
      double dim = v.GetDouble();
      if ((*it)->mType == TelemetryDimension::kRange) {
        if (dim >= (*it)->mRange.first && dim <= (*it)->mRange.second) {
          p /= boost::lexical_cast<string>(dim);
        } else {
          p /= kOther;
        }
      } else {
        // todo log error - numbers can only use a range comparison
      }
    }
  }
  return p;
}

////////////////////////////////////////////////////////////////////////////////
/// Private Member Functions
////////////////////////////////////////////////////////////////////////////////
void
TelemetrySchema::LoadDimensions(const rapidjson::Document& aDoc)
{
  const rapidjson::Value& dimensions = aDoc["dimensions"];
  if (!dimensions.IsArray()) {
    throw runtime_error("dimensions element must be an array");
  }
  for (rapidjson::Value::ConstValueIterator it = dimensions.Begin();
       it != dimensions.End(); ++it) {
    if (!it->IsObject()) {
      throw runtime_error("dimension elemenst must be objects");
    }
    try {
      shared_ptr<TelemetryDimension> dim(new TelemetryDimension(*it));
      mDimensions.push_back(dim);
    }
    catch (exception& e) {
      stringstream ss;
      ss << "invalid dimension schema: " << e.what();
      throw runtime_error(ss.str());
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
std::string TelemetrySchema::SafePath(const std::string& s)
{
  namespace bx = boost::xpressive;
  static bx::sregex clean_re = ~bx::set[bx::range('a', 'z') |
                                          bx::range('A', 'Z') |
                                          bx::range('0', '9') |
                                          '_' | '/' | '.'];
  return bx::regex_replace(s, clean_re, "_");
}

}
}
