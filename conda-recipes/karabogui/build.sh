#!/usr/bin/env bash
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

# Install karaboCommon
export BUILD_KARABO_SUBMODULE=NATIVE
cd ./src/pythonKarabo
python setup.py install
unset BUILD_KARABO_SUBMODULE
# Install pythonGui
cd ../pythonGui
python setup.py install

# from 
# https://docs.conda.io/projects/conda-build/en/latest/resources/activate-scripts.html
# Copy the [de]activate scripts to $PREFIX/etc/conda/[de]activate.d.
# This will allow them to be run on environment activation.
for CHANGE in "activate" "deactivate"
do
    TARGET_SCRIPT_DIR="${PREFIX}/etc/conda/${CHANGE}.d"
    mkdir -p $TARGET_SCRIPT_DIR
    cp "${RECIPE_DIR}/env_scripts/${CHANGE}.sh" "${TARGET_SCRIPT_DIR}/${PKG_NAME}_${CHANGE}.sh"
done
