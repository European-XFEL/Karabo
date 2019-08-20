#!/bin/bash
BUILD_OPTION=$2
OS=$(uname -s)
CWD=$(pwd)
DIST=dist/$OS

KARABO=$(readlink -f "$1")
PYTHON=$KARABO/extern/bin/python
PIP=$KARABO/extern/bin/pip
PIP_EXTRA_ARGS=

# clean previous dist folder - in case of file/folders deletion/creation in original source folder
rm -rf dist
mkdir -p $DIST/lib
mkdir -p $DIST/bin
cd $DIST/bin

cd ../../../../../../src/pythonKarabo
rm -rf dist/ build/
if [ "$BUILD_OPTION" == "normal" ]; then
    # Run the setup.py script first to generate versioning data!
    $PYTHON setup.py egg_info
    $PIP --disable-pip-version-check install -U $PIP_EXTRA_ARGS .
else
    $PIP --disable-pip-version-check install $PIP_EXTRA_ARGS -e .
fi
cd $CWD
