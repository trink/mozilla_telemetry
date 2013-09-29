Prerequisites {#mainpage}
====
* Clang 3.1 or GCC 4.7.0 or Visual Studio 10
* CMake (2.8.7+) - http://cmake.org/cmake/resources/software.html
* Boost (1.51.0) - http://www.boost.org/users/download/
* zlib
* OpenSSL
* Protobuf

Optional (used for documentation)
----
* Graphviz (2.28.0) - http://graphviz.org/Download..php
* Doxygen (1.8+)- http://www.stack.nl/~dimitri/doxygen/download.html#latestsrc

telemetry  - UNIX Build Instructions
====
    git clone https://github.com/trink/mozilla_telemetry.git
    cd mozilla_telemetry 
    mkdir release
    cd release
    cmake -DCMAKE_BUILD_TYPE=release ..
    make

Configuring the converter
====
input_directory (string) - Directory monitored by the converter for new files.
telemetry_schema (string) - JSON file containing the dimension mapping.
histogram_server (string) - Hostname:port of the histogram.json web service.
storage_path (string) - Converter output directory
upload_path (string) - Staging directory for S3 uploads.
max_uncompressed (int) - Maximum uncompressed size of a telemetry record.
memory_constraint (int) - 
compression_preset (int) -


    {
        "input_directory": "./input",
        "telemetry_schema": "../common/test/data/telemetry_schema.json",
        "histogram_server": "localhost:9898",
        "storage_path": "./storage",
        "log_path": "./log",
        "upload_path": "./upload",               
        "max_uncompressed": 1048576,
        "memory_constraint": 1000,
        "compression_preset": 0
    }


Setting up/running the histogram server
====
    git clone git@github.com:mreid-moz/telemetry-server.git
    cd telemetry-server
    ./get_histogram_tools.sh
    python histogram_server.py

Running the converter
====
*in the release directory*

    mkdir input
    ./convert ..lconvert.json

*from another shell, in the release directory*

    cp ../common/test/data/telemetry1.log input/

Without the histogram server running it will produce something like this:

    processing file:telemetry1.log
    LoadHistogram - connect: Connection refused
    ConvertHistogramData - histogram not found: http://hg.mozilla.org/releases/mozilla-release/rev/a55c55edf302
    Conversion failed: 17caa68c-25f5-450a-9cce-7d31318846d8/idle-daily/Firefox/23.0.1/release/20130814063812
    done processing file:telemetry1.log records:1 failed conversions:1 0.002733

With the histogram server running:

    processing file:telemetry1.log
    done processing file:telemetry1.log records:1 failed conversions:0 0.010078

Ubuntu Notes
====
apt-get install cmake libprotoc-dev zlib1g-dev libboost-system1.53-dev libboost-system1.53-dev libboost-system1.53-dev 
               libboost-filesystem1.53-dev libboost-thread1.53-dev libboost-test1.53-dev
