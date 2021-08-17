#!/bin/bash

if [ -z $KARABO ]; then
  echo "\$KARABO is not defined. Make sure you have sourced the activate script for the Karabo Framework you would like to use."
  exit 1
fi
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
# "karabo install" expects the installation directory here
TARGET_DIR=$SCRIPTPATH/dist/$1/cmake/.
BUILD_DIR=$SCRIPTPATH/build
TESTS_DIR=$BUILD_DIR/__PACKAGE_NAME__

mkdir -p $TARGET_DIR
rm -fr $BUILD_DIR
mkdir -p $BUILD_DIR

cmake \
    -DCMAKE_BUILD_TYPE=$1 \
    -DBoost_NO_BOOST_CMAKE=ON \
    -DBoost_NO_SYSTEM_PATHS=ON \
    -DCMAKE_PREFIX_PATH=$KARABO/extern \
    -DCMAKE_INSTALL_PREFIX=$TARGET_DIR \
    -B $BUILD_DIR .
cd $BUILD_DIR
cmake --build .
cd $TESTS_DIR
ctest -VV
