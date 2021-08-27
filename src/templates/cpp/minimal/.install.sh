#!/bin/bash

if [ -z $KARABO ]; then
  echo "\$KARABO is not defined. Make sure you have sourced the activate script for the Karabo Framework you would like to use."
  exit 1
fi
SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
# "karabo install" expects the installation directory here
TARGET_DIR=$SCRIPTPATH/dist/$1/cmake/.
BUILD_DIR=$SCRIPTPATH/build

mkdir -p $TARGET_DIR
mkdir -p $BUILD_DIR

# handle the make -j option from the caller.
BUILD_OPT="--build ."
unset MAKEFLAGS
if [[ "${2}" != "" ]]; then
  BUILD_OPT="--build . -j ${2}"
fi


cmake \
    -DCMAKE_BUILD_TYPE=$1 \
    -DBoost_NO_BOOST_CMAKE=ON \
    -DBoost_NO_SYSTEM_PATHS=ON \
    -DCMAKE_PREFIX_PATH=$KARABO/extern \
    -DCMAKE_INSTALL_PREFIX=$TARGET_DIR \
    -B $BUILD_DIR .
cd $BUILD_DIR
cmake $BUILD_OPT
$(cp $BUILD_DIR/__PACKAGE_NAME__/lib*.so $TARGET_DIR | true)

