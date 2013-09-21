Prerequisites {#mainpage}
====
* Clang 3.1 or GCC 4.7.0 or Visual Studio 10
* CMake (2.8.7+) - http://cmake.org/cmake/resources/software.html
* Boost (1.51.0) - http://www.boost.org/users/download/

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
