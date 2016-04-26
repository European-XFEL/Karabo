#!/bin/bash
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
$PYTHON setup.py bdist_wheel
$PIP install -U $PIP_EXTRA_ARGS dist/*.whl
cd $CWD
