#!/usr/bin/env bash
# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

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
