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
#include "Metric.h"
#include "message.pb.h"

#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <chrono>
#include <csignal>
#include <exception>
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sstream>
#include <sys/inotify.h>

using namespace std;
namespace fs = boost::filesystem;
namespace mt = mozilla::telemetry;

struct ConvertConfig
{
  fs::path    mInputDirectory;
  fs::path    mTelemetrySchema;
  std::string mHistogramServer;
  fs::path    mStoragePath;
  fs::path    mLogPath;
  fs::path    mUploadPath;
  uint64_t    mMaxUncompressed;
  size_t      mMemoryConstraint;
  int         mCompressionPreset;
};


///////////////////////////////////////////////////////////////////////////////
void ReadConfig(const char* aFile, ConvertConfig& aConfig)
{
  ifstream ifs(aFile);
  if (!ifs) {
    stringstream ss;
    ss << "file open failed: " << aFile;
    throw runtime_error(ss.str());
  }
  string json((istream_iterator<char>(ifs)), istream_iterator<char>());

  RapidjsonDocument doc;
  if (doc.Parse<0>(json.c_str()).HasParseError()) {
    stringstream ss;
    ss << "json parse failed: " << doc.GetParseError();
    throw runtime_error(ss.str());
  }

  RapidjsonValue& idir = doc["input_directory"];
  if (!idir.IsString()) {
    throw runtime_error("input_directory not specified");
  }
  aConfig.mInputDirectory = idir.GetString();
  if (!exists(aConfig.mInputDirectory)) {
    create_directories(aConfig.mInputDirectory);
  }

  RapidjsonValue& ts = doc["telemetry_schema"];
  if (!ts.IsString()) {
    throw runtime_error("telemetry_schema not specified");
  }
  aConfig.mTelemetrySchema = ts.GetString();

  RapidjsonValue& hs = doc["histogram_server"];
  if (!hs.IsString()) {
    throw runtime_error("histogram_server not specified");
  }
  aConfig.mHistogramServer = hs.GetString();

  RapidjsonValue& sp = doc["storage_path"];
  if (!sp.IsString()) {
    throw runtime_error("storage_path not specified");
  }
  aConfig.mStoragePath = sp.GetString();
  if (!exists(aConfig.mStoragePath)) {
    create_directories(aConfig.mStoragePath);
  }

  RapidjsonValue& lp = doc["log_path"];
  if (!lp.IsString()) {
    throw runtime_error("log_path not specified");
  }
  aConfig.mLogPath = lp.GetString();
  if (!exists(aConfig.mLogPath)) {
    create_directories(aConfig.mLogPath);
  }

  RapidjsonValue& up = doc["upload_path"];
  if (!up.IsString()) {
    throw runtime_error("upload_path not specified");
  }
  aConfig.mUploadPath = up.GetString();
  if (!exists(aConfig.mUploadPath)) {
    create_directories(aConfig.mUploadPath);
  }

  RapidjsonValue& mu = doc["max_uncompressed"];
  if (!mu.IsUint64()) {
    throw runtime_error("max_uncompressed not specified");
  }
  aConfig.mMaxUncompressed = mu.GetUint64();

  RapidjsonValue& mc = doc["memory_constraint"];
  if (!mc.IsUint()) {
    throw runtime_error("memory_constraint not specified");
  }
  aConfig.mMemoryConstraint = mc.GetUint();

  RapidjsonValue& cpr = doc["compression_preset"];
  if (!cpr.IsInt()) {
    throw runtime_error("compression_preset not specified");
  }
  aConfig.mCompressionPreset = cpr.GetInt();
}

///////////////////////////////////////////////////////////////////////////////
bool ProcessFile(const boost::filesystem::path& aName,
                 const mt::TelemetrySchema& aSchema, mt::HistogramCache& aCache,
                 mt::RecordWriter& aWriter)
{
  try {
    cout << "processing file:" << aName.filename() << endl;
    chrono::time_point<chrono::system_clock> start, end;
    start = chrono::system_clock::now();
    ifstream file(aName.c_str());
    mt::TelemetryRecord tr;
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    int cnt = 0, failures = 0;
    while (tr.Read(file)) {
      if (ConvertHistogramData(aCache, tr.GetDocument())) {
        tr.GetDocument().Accept(writer);
        boost::filesystem::path p = aSchema.GetDimensionPath(tr.GetDocument());
        aWriter.Write(p, sb.GetString(), sb.GetSize());
      } else {
        // cerr << "Conversion failed: " << tr.GetPath() << endl;
        ++failures;
      }
      ++cnt;
    }
    end = chrono::system_clock::now();
    chrono::duration<double> elapsed = end - start;
    double throughput = 0;
    double duration = elapsed.count();
    if (duration > 0) {
      throughput = file_size(aName) / 1024 / 1024 / duration;
    }
    cout << "done processing file:" << aName.filename() << " success:" << cnt
      << " failures:" << failures << " time:" << duration
      << " throughput MiB/s:" << throughput << endl;
  }
  catch (const exception& e) {
    cerr << "ProcessFile std exception: " << e.what() << endl;
    return false;
  }
  catch (...) {
    cerr << "ProcessFile unknown exception\n";
    return false;
  }
  return true;
}

static sig_atomic_t gStop = 0;
////////////////////////////////////////////////////////////////////////////////
void shutdown(int sig)
{
  gStop = 1;
  signal(sig, SIG_DFL);
}

////////////////////////////////////////////////////////////////////////////////
ofstream& RollLog(ofstream& aOutput, const ConvertConfig& config)
{
  bool needsRolling = false;
  if (!aOutput || !aOutput.is_open() || needsRolling) {
    if (aOutput.is_open()) {
      aOutput.close();
    }
    // todo create time based name i.e. convert-20130927.log
    fs::path fn(config.mLogPath / "convert.log");
    aOutput.open(fn.c_str(), ios::binary | ios::app);
  }
  return aOutput;
}

const size_t kMaxEventSize = sizeof(struct inotify_event) + FILENAME_MAX + 1;
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  if (argc < 2) {
    cerr << "usage: " << argv[0] << " <json config> <space-separated batch file list or nothing for inotify>\n";
    return EXIT_FAILURE;
  }

  try {
    ConvertConfig config;
    ReadConfig(argv[1], config);
    mt::HistogramCache cache(config.mHistogramServer);
    mt::TelemetrySchema schema(config.mTelemetrySchema);
    mt::RecordWriter writer(config.mStoragePath, config.mUploadPath,
                            config.mMaxUncompressed, config.mMemoryConstraint,
                            config.mCompressionPreset);

    for (int i = 2; i < argc; i++) {
      ProcessFile(argv[i], schema, cache, writer);
    }
    // do not move on to inotify mode in batch mode
    if (argc > 2) return EXIT_SUCCESS;

    signal(SIGHUP, shutdown);
    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);
    signal(SIGUSR2, shutdown);

    int notify = inotify_init();
    if (notify < 0) {
      cerr << "inotify_init failed\n";
      return EXIT_FAILURE;
    }
    int watch = inotify_add_watch(notify, config.mInputDirectory.c_str(),
                                  IN_CLOSE_WRITE | IN_MOVED_TO);

    message::Message metrics;
    metrics.set_type("telemetry");
    metrics.set_logger("telemetry_converter");
    metrics.set_pid(0);
    metrics.set_hostname("todo");
    int bytesRead;
    char buf[kMaxEventSize];
    ofstream ofs;
    while (!gStop) {
      fs::path fn;
      for (fs::directory_iterator it(config.mInputDirectory);
           it != fs::directory_iterator(); ++it) {
        if (is_regular_file(*it)) {
          fn = *it;
          break;
        }
      }

      if (!fn.empty()) {
        try {
          fs::path tfn = fs::temp_directory_path() / fn.filename();
          rename(fn, tfn);
          if (ProcessFile(tfn, schema, cache, writer)) remove(tfn);
          boost::uuids::uuid u = boost::uuids::random_generator()();
          metrics.set_uuid(&u, u.size());
          metrics.set_timestamp(0);
          cache.GetMetrics(metrics);
          RollLog(ofs, config);
          mt::WriteMessage(ofs, metrics);
        }
        catch (const exception& e) {
          cerr << "Rename failed:" << fn.filename()
            << ", someone beat us to it\n";
        }
        continue;
      }

      // block waiting for a new file
      bytesRead = read(notify, buf, kMaxEventSize);
      if (bytesRead < 0) break;
      // discard the event, it was just needed to trigger the scan
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
  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();

  return EXIT_SUCCESS;
}
