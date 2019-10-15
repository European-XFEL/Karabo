#!/usr/bin/env bash

CONDA_CHANNEL_PATH=/var/www/html/karabo/channel

# This script is responsible for building our karabogui conda package
# and mirroring all the needed packages into a directory

set -e
set -o pipefail

. ci/utils/enable_internet.sh

pushd ./src/pythonGui/scripts/

# If everything goes well, our package will be inside conda-recipe/package/
# Now we just need to upload it to our channel
bash ./build_conda_recipe.sh

# Create a test environment for the created package
conda remove -n karabogui_test --yes --all
conda create -n karabogui_test --yes
source activate karabogui_test
conda install karabogui=${CI_COMMIT_REF_NAME} \
    --override-channels \
    -c local \
    -c http://exflserv05.desy.de/karabo/channel \
    -c conda-forge \
    -c conda-forge/label/cf201901 \
    -c defaults \
    -c anaconda \

# Create and populate the mirrors inside ./mirror/
python ./create_mirror_channels.py --target_dir ~/mirror/ --env karabogui_test

popd

. ci/utils/disable_internet.sh