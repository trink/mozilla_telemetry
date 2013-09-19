/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Telemetry data coverter implementation @file

#include "HistogramCache.h"
#include "HistogramConverter.h"
#include "TelemetryReader.h"
#include "TelemetrySchema.h"

#include <boost/scoped_array.hpp>
#include <boost/filesystem.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <sstream>
#include <thread>

using namespace std;
namespace fs = boost::filesystem;
namespace mt = mozilla::telemetry;

struct ConvertConfig
{
  ConvertConfig() :
    mInputPollMs(0) { }

  fs::path mInputFile; // empty for cin
  fs::path mTelemetrySchema;
  fs::path mCachePath;
  fs::path mStoragePath;
  fs::path mLogPath;

  int mInputPollMs; // 0 for no polling, exits when there are no more records
};

void read_config(const char* aFile, ConvertConfig& aConfig)
{
  ifstream ifs(aFile, ifstream::binary);
  if (!ifs) {
    stringstream ss;
    ss << "file open failed: " << aFile;
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
  rapidjson::Value& v = doc["input_file"];
  if (!v.IsString()) {
    throw runtime_error("input_file not specified");
  }
  aConfig.mInputFile = v.GetString();

  v = doc["input_poll_ms"];
  if (v.IsInt()) {
    aConfig.mInputPollMs = v.GetInt();
  }

  v = doc["telemetry_schema"];
  if (!v.IsString()) {
    throw runtime_error("telemetry_schema not specified");
  }
  aConfig.mTelemetrySchema = v.GetString();

  v = doc["cache_path"];
  if (!v.IsString()) {
    throw runtime_error("cache_path not specified");
  }
  aConfig.mCachePath = v.GetString();

  v = doc["storage_path"];
  if (!v.IsString()) {
    throw runtime_error("storage_path not specified");
  }
  aConfig.mStoragePath = v.GetString();

  v = doc["log_path"];
  if (!v.IsString()) {
    throw runtime_error("log_path not specified");
  }
  aConfig.mLogPath = v.GetString();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
  if (argc != 2) {
    cerr << "usage: " << argv[0] << " <json config>\n";
    return EXIT_FAILURE;
  }

  try {
    ConvertConfig config;
    read_config(argv[1], config);
    mt::HistogramCache cache(config.mCachePath);
    mt::TelemetrySchema schema(config.mTelemetrySchema);

    ifstream file;
    istream* is = &file;
    if (config.mInputFile.empty()) {
      is = &cin;
    } else {
      file.open(config.mInputFile.c_str(), ios_base::binary);
    }
    mt::TelemetryReader reader(*is);
    mt::TelemetryRecord tr;
    while (true) {
      if (!reader.Read(tr)) {
        if (config.mInputPollMs) {
          this_thread::sleep_for(chrono::milliseconds(config.mInputPollMs));
          continue;
        } else {
          break;
        }
      }
      ConvertHistogramData(cache, tr.mDocument);
      boost::filesystem::path p = config.mStoragePath /
        schema.GetDimensionPath(tr.mDocument["info"]) / "todo_001.log";
      cout << p << endl;
      // todo create path
      // create/append/roll log file
      // write uuid\tjson\n
    }
  }
  catch (const exception& e) {
    cerr << "std exception: " << e.what();
    return EXIT_FAILURE;
  }
  catch (...) {
    cerr << "unknown exception";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
