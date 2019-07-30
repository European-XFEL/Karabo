#!/bin/bash
BUILD_OPTION=$2
CWD=$(pwd)
OS=$(uname -s)
DIST=dist/$OS

KARABO=$(readlink -f "$1")
PYTHON=$KARABO/extern/bin/python
PIP=$KARABO/extern/bin/pip
PIP_EXTRA_ARGS=

# clean previous dist folder - in case of file/folders deletion/creation in original source folder
rm -rf dist
mkdir -p $DIST/lib
mkdir -p $DIST/bin


cd ../../../src/pythonGui
rm -rf dist/ build/
if [ "$BUILD_OPTION" == "normal" ]; then
    # XXX: `pip install .` raises an exception! Just run setup.py
    $PYTHON setup.py install
else
    $PIP --disable-pip-version-check install $PIP_EXTRA_ARGS -e .
fi
cd $CWD
