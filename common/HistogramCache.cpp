/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/// @brief Histogram cache implementation @file

#include "HistogramCache.h"

#include <boost/asio.hpp>

#include <openssl/md5.h>
#include <iostream>

using namespace std;
namespace fs = boost::filesystem;

namespace mozilla {
namespace telemetry {

////////////////////////////////////////////////////////////////////////////////
HistogramCache::HistogramCache(const std::string& aCacheHost)
{
  size_t pos = aCacheHost.find(':');
  mCacheHost = aCacheHost.substr(0, pos);
  if (pos != string::npos) {
    mCachePort = aCacheHost.substr(pos + 1);
  } else {
    mCachePort = "http";
  }
}

////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<Histogram>
HistogramCache::FindHistogram(const std::string& aRevisionKey)
{
  shared_ptr<Histogram> h;

  if (aRevisionKey.compare(0, 4, "http") != 0) {
    return h;
  }
  auto it = mRevisions.find(aRevisionKey);
  if (it != mRevisions.end()) {
    h = it->second;
  } else {
    try {
      h = LoadHistogram(aRevisionKey); 
    }
    catch (const exception& e) {
      cerr << "LoadHistogram - " << e.what() << endl;
    }
  }
  return h;
}

////////////////////////////////////////////////////////////////////////////////
/// Private Member Functions
////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<Histogram>
HistogramCache::LoadHistogram(const std::string& aRevisionKey)
{
  using boost::asio::ip::tcp;
  boost::asio::io_service io_service;

  // Get a list of endpoints corresponding to the server name.
  tcp::resolver resolver(io_service);
  tcp::resolver::query query(mCacheHost, mCachePort);
  tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

  // Try each endpoint until we successfully establish a connection.
  tcp::socket socket(io_service);
  boost::asio::connect(socket, endpoint_iterator);

  // Form the request. We specify the "Connection: close" header so that the
  // server will close the socket after transmitting the response. This will
  // allow us to treat all data up until the EOF as the content.
  boost::asio::streambuf request;
  std::ostream request_stream(&request);
  request_stream << "GET " << "/histogram_buckets?revision=" << aRevisionKey
    << " HTTP/1.0\r\n";
  request_stream << "Host: " << mCacheHost << "\r\n";
  request_stream << "Accept: */*\r\n";
  request_stream << "Connection: close\r\n\r\n";

  // Send the request.
  boost::asio::write(socket, request);

  // Read the response status line. The response streambuf will automatically
  // grow to accommodate the entire line. The growth may be limited by passing
  // a maximum size to the streambuf constructor.
  boost::asio::streambuf response;
  boost::asio::read_until(socket, response, "\r\n");

  // Check that response is OK.
  std::istream response_stream(&response);
  std::string http_version;
  response_stream >> http_version;
  unsigned int status_code;
  response_stream >> status_code;
  std::string status_message;
  std::getline(response_stream, status_message);
  if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
    cerr << "LoadHistogram - invalid server response\n";
    return shared_ptr<Histogram>();
  }

  // Read the response headers, which are terminated by a blank line.
  boost::asio::read_until(socket, response, "\r\n\r\n");
  
  // Process the response headers.
  std::string header;
  while (std::getline(response_stream, header) && header != "\r");
  if (status_code != 200) {
    cerr << "LoadHistogram - non 200 response\n";
    shared_ptr<Histogram> h;
    mRevisions.insert(make_pair(aRevisionKey, h)); // prevent retries
    return h;
  }

  ostringstream oss;
  // Write whatever content we already have to output.
  if (response.size() > 0) oss << &response;
  // Read until EOF, writing data to output as we go.
  boost::system::error_code error;
  while (boost::asio::read(socket, response,
                           boost::asio::transfer_at_least(1), error)) {
    oss << &response;
  }
  if (error != boost::asio::error::eof) {
    throw boost::system::system_error(error);
  }
  try {
    string json = oss.str();
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5((unsigned char*)json.c_str(), json.size(), digest);
    shared_ptr<Histogram> h(new Histogram(json));
    string key(reinterpret_cast<char*>(digest), MD5_DIGEST_LENGTH);
    auto p = mCache.insert(make_pair(key, h));
    mRevisions.insert(make_pair(aRevisionKey, h));
    return p.first->second;
  }
  catch (exception &e){
    cerr << "LoadHistogram - bad histogram specification\n" << oss.str();
  }
  return shared_ptr<Histogram>();
}

}
}
