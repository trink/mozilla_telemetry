/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Telemetry data coverter implementation @file

#include "HistogramCache.h"
#include "HistogramConverter.h"
#include "TelemetryRecord.h"
#include "TelemetrySchema.h"
#include "RecordWriter.h"

#include <boost/scoped_array.hpp>
#include <boost/filesystem.hpp>
#include <csignal>
#include <exception>
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sstream>
#include <sys/inotify.h>
#include <thread>

using namespace std;
namespace fs = boost::filesystem;
namespace mt = mozilla::telemetry;

struct ConvertConfig
{
  fs::path  mInputDirectory;
  fs::path  mTelemetrySchema;
  fs::path  mCachePath;
  fs::path  mStoragePath;
  fs::path  mLogPath;
  fs::path  mUploadPath;
  uint64_t  mMaxUncompressed;
  size_t    mMemoryConstraint;
  int      mCompressionPreset;
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

  rapidjson::Value& idir = doc["input_directory"];
  if (!idir.IsString()) {
    throw runtime_error("input_directory not specified");
  }
  aConfig.mInputDirectory = idir.GetString();

  rapidjson::Value& ts = doc["telemetry_schema"];
  if (!ts.IsString()) {
    throw runtime_error("telemetry_schema not specified");
  }
  aConfig.mTelemetrySchema = ts.GetString();

  rapidjson::Value& cp = doc["cache_path"];
  if (!cp.IsString()) {
    throw runtime_error("cache_path not specified");
  }
  aConfig.mCachePath = cp.GetString();

  rapidjson::Value& sp = doc["storage_path"];
  if (!sp.IsString()) {
    throw runtime_error("storage_path not specified");
  }
  aConfig.mStoragePath = sp.GetString();

  rapidjson::Value& lp = doc["log_path"];
  if (!lp.IsString()) {
    throw runtime_error("log_path not specified");
  }
  aConfig.mLogPath = lp.GetString();

  rapidjson::Value& up = doc["upload_path"];
  if (!up.IsString()) {
    throw runtime_error("upload_path not specified");
  }
  aConfig.mUploadPath = up.GetString();

  rapidjson::Value& mu = doc["max_uncompressed"];
  if (!mu.IsUint64()) {
    throw runtime_error("max_uncompressed not specified");
  }
  aConfig.mMaxUncompressed = mu.GetUint64();

  rapidjson::Value& mc = doc["memory_constraint"];
  if (!mc.IsUint()) {
    throw runtime_error("memory_constraint not specified");
  }
  aConfig.mMemoryConstraint = mc.GetUint();

  rapidjson::Value& cpr = doc["compression_preset"];
  if (!cpr.IsInt()) {
    throw runtime_error("compression_preset not specified");
  }
  aConfig.mCompressionPreset = cpr.GetInt();
}

///////////////////////////////////////////////////////////////////////////////
void ProcessFile(const ConvertConfig& config, const char* aName,
                 const mt::TelemetrySchema& aSchema, mt::HistogramCache& aCache,
                 mt::RecordWriter& aWriter)
{
  try {
    fs::path fn = config.mInputDirectory / aName;
    fs::path tfn = fs::temp_directory_path() / aName;
    rename(fn, tfn);
    cout << "processing file:" << aName << endl;
    ifstream file(tfn.c_str());
    mt::TelemetryRecord tr;
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    while (tr.Read(file)) {
      if (ConvertHistogramData(aCache, tr.GetDocument())) {
        tr.GetDocument().Accept(writer);
        boost::filesystem::path p = aSchema.GetDimensionPath(tr.GetDocument());
        aWriter.Write(p, sb.GetString(), sb.Size());
      } else {
        cerr << "Conversion failed: " << tr.GetPath() << endl;
      }
    }
    remove(tfn);
  }
  catch (const exception& e) {
    cerr << "ProcessFile std exception: " << e.what();
  }
  catch (...) {
    cerr << "Process unknown exception";
  }
}

static sig_atomic_t gStop = 0;
////////////////////////////////////////////////////////////////////////////////
void shutdown(int)
{
  gStop = 1;
}

const size_t kMaxEventSize = sizeof(struct inotify_event) + FILENAME_MAX + 1;
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
  if (argc != 2) {
    cerr << "usage: " << argv[0] << " <json config>\n";
    return EXIT_FAILURE;
  }
  signal(SIGHUP, shutdown);
  signal(SIGINT, shutdown);
  signal(SIGTERM, shutdown);
  signal(SIGUSR2, shutdown);

  try {
    ConvertConfig config;
    read_config(argv[1], config);
    mt::HistogramCache cache(config.mCachePath);
    mt::TelemetrySchema schema(config.mTelemetrySchema);
    mt::RecordWriter writer(config.mStoragePath, config.mUploadPath,
                            config.mMaxUncompressed, config.mMemoryConstraint,
                            config.mCompressionPreset);

    int notify = inotify_init();
    if (notify < 0) {
      cerr << "inotify_init failed\n";
      return EXIT_FAILURE;
    }
    int watch = inotify_add_watch(notify, config.mInputDirectory.c_str(),
                                  IN_CLOSE_WRITE);
    int bytesRead;
    char buf[kMaxEventSize];
    while (!gStop) {
      bytesRead = read(notify, buf, kMaxEventSize);
      if (bytesRead < 0) break;

      int i = 0;
      while (i < bytesRead) {
        inotify_event* event = reinterpret_cast<inotify_event*>(&buf[i]);
        if (event->len) { // event->mask & IN_CLOSE_WRITE
          ProcessFile(config, event->name, schema, cache, writer);
        }
        i += sizeof(struct inotify_event) + event->len;
      }
    }
    inotify_rm_watch(notify, watch);
    close(notify);
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
