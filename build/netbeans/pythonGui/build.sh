#!/bin/bash
BUILD_OPTION=$2
CWD=$(pwd)
OS=$(uname -s)
DIST=dist/$OS

if [ "$OS" = "Darwin" ]; then
    PYTHON=/opt/local/bin/python
    KARABO=$($PYTHON -c 'import os,sys;print(os.path.realpath(sys.argv[1]))' "$1")
    PIP=/opt/local/bin/pip
    PIP_EXTRA_ARGS="--user"
else
    KARABO=$(readlink -f "$1")
    PYTHON=$KARABO/extern/bin/python
    PIP=$KARABO/extern/bin/pip
    PIP_EXTRA_ARGS=
fi

# clean previous dist folder - in case of file/folders deletion/creation in original source folder
rm -rf dist
mkdir -p $DIST/lib
mkdir -p $DIST/bin


cd ../../../src/pythonGui
rm -rf dist/ build/
if [ "$BUILD_OPTION" == "wheel" ]; then
    $PYTHON setup.py bdist_wheel
    $PIP install --disable-pip-version-check -U $PIP_EXTRA_ARGS dist/*.whl
else
    $PYTHON setup.py develop
fi
cd $CWD
