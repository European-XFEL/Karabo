#!/usr/bin/env bash

# Install karaboCommon
export BUILD_KARABO_GUI=1
cd ./src/pythonKarabo
python setup.py install

# Install pythonGui
cd ../pythonGui
python setup.py install