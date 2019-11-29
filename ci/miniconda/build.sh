#!/usr/bin/env bash

# This script is responsible for building our karabogui conda package
# and mirroring all the needed packages into a directory

set -e
set -o pipefail

. ci/utils/enable_internet.sh

printAndRun() {
    typeset cmnd="$*"
    echo cmnd=$cmnd
    eval $cmnd
}

. ci/utils/enable_internet.sh

printAndRun pushd ./src/pythonGui/
printAndRun conda devenv
printAndRun source activate karabogui

# Generate _version file
printAndRun python setup.py --version

# Generate meta.yaml recipe information
printAndRun conda run -n karabogui python -m cogapp -d -o ./conda-recipe/meta.yaml ./conda-recipe/meta_base.yaml

echo "**********Building KaraboGui with the following Recipe**********"
cat ./conda-recipe/meta.yaml

# 6. Build the package
printAndRun conda build ./conda-recipe/ -c http://exflserv05.desy.de/karabo/channel \
               -c conda-forge \
               -c defaults \
               --no-anaconda-upload

# Create mirror channel locally
rm -rf /tmp/mirror/
printAndRun python ./scripts/create_mirror_channels.py --target_dir /tmp/mirror/ --env karabogui

popd

. ci/utils/disable_internet.sh