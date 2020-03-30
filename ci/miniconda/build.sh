#!/usr/bin/env bash

# This script is responsible for building our karabogui conda package
# and mirroring all the needed packages into a directory

set -e
set -o pipefail

printAndRun() {
    typeset cmnd="$*"
    echo cmnd=$cmnd
    eval $cmnd
}


printAndRun build_conda_env.sh clean install
# Generate meta.yaml recipe information
printAndRun conda run -n karabogui python -m cogapp -d -o ./conda-recipes/karabogui/meta.yaml ./conda-recipes/karabogui/meta_base.yaml

echo "**********Building KaraboGui with the following Recipe**********"
cat ./conda-recipes/karabogui/meta.yaml

# 6. Build the package
printAndRun conda build ./conda-recipes/karabogui/. -c http://exflserv05.desy.de/karabo/channel \
               -c conda-forge \
               -c defaults \
               --no-anaconda-upload

# Create mirror channel locally
rm -rf /tmp/mirror/
printAndRun python ./conda-recipes/scripts/create_mirror_channels.py --target_dir /tmp/mirror/ --env karabogui

popd
