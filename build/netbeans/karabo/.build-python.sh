#!/bin/bash

# BUILD_OPTION is either "normal" or "develop" as defined in `auto_build_all.sh`
# and transported through `build/netbeans/karabo/Makefile`
BUILD_OPTION=$2

PYTHON=$(readlink -f "$1")/extern/bin/python

pushd ../../../src/pythonKarabo
rm -rf dist/ build/
if [ "$BUILD_OPTION" == "normal" ]; then
    $PYTHON setup.py bdist_wheel
    $PYTHON -m pip --disable-pip-version-check install -U dist/*.whl
else
    $PYTHON -m pip --disable-pip-version-check install -e .
fi
popd
