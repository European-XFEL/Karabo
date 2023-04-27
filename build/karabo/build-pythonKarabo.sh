#!/bin/bash
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
scriptDir=$(dirname `[[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"`)
BUILD_OPTION=$2
OS=$(uname -s)
CWD=$(pwd)
DIST=$scriptDir/dist/$OS

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
rm -rf $scriptDir/dist
mkdir -p $DIST/lib
mkdir -p $DIST/bin
cd $DIST/bin

cd $scriptDir/../../src/pythonKarabo

rm -rf dist/ build/

if [ "$BUILD_OPTION" == "normal" ]; then
    $PYTHON setup.py bdist_wheel
    $PIP --disable-pip-version-check install -U $PIP_EXTRA_ARGS dist/*.whl
else
    $PIP --disable-pip-version-check install $PIP_EXTRA_ARGS -e .
fi

cd $CWD
