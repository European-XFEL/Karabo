#!/usr/bin/env bash

# Install karaboCommon
export BUILD_KARABO_SUBMODULE=MDL
cd ./src/pythonKarabo
python setup.py install
unset BUILD_KARABO_SUBMODULE
