******************************
__CLASS_NAME__ Device (C++)
******************************

Compiling
=========

1. Source activate the Karabo installation against which the device will be
   developed:

    ``cd <Karabo installation root directory>``
    ``source ./activate``

2. Goto the device source root directory and generate its build files with cmake:

     ``cd $KARABO/devices/__PACKAGE_NAME__``
     ``mkdir build``
     ``cd build``
     ``cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$KARABO;$KARABO/extern" ..``

   CMAKE_BUILD_TYPE can also be set to ``Release``.

   The double quotes around the value of ``CMAKE_PREFIX_PATH`` are required.


3. Build the device:

     ``cd $KARABO/devices/__PACKAGE_NAME__/build``
     ``cmake --build . ``

   ``make`` can also be used as long as the Makefile generator is used by cmake.

Testing
=======

To run the tests, go to the tests directory in your build tree and use ``ctest``:

    ``cd $KARABO/devices/__PACKAGE_NAME__/build/__PACKAGE_NAME__``
    ``ctest -VV``
