#!/bin/bash

# BUILD_OPTION is either "normal" or "develop" as defined in `auto_build_all.sh`
# and transported through `build/netbeans/karabo/Makefile`
BUILD_OPTION=$2

OS=$(uname -s)
DIST=dist/$OS

KARABO=$(readlink -f "$1")
PYTHON=$KARABO/extern/bin/python
PIP=$KARABO/extern/bin/pip


# clean previous dist folder - in case of file/folders deletion/creation in original source folder
rm -rf dist
mkdir -p $DIST/lib
mkdir -p $DIST/bin
pushd $DIST/bin/../../../../../../src/pythonKarabo
rm -rf dist/ build/
if [ "$BUILD_OPTION" == "normal" ]; then
    $PYTHON setup.py bdist_wheel
    $PIP --disable-pip-version-check install -U dist/*.whl
else
    $PIP --disable-pip-version-check install -e .
fi
popd
