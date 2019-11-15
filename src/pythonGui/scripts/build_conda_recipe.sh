#!/usr/bin/env bash

# Exit shell if anything fails
set -e
set -o pipefail

SCRIPTPATH=$(dirname $(readlink -f "$0"))
echo ${SCRIPTPATH}

cd ${SCRIPTPATH}/../

# 1. Activate karabogui environment
conda devenv
source activate karabogui

# 2. Uninstall any previous versions
pip uninstall KaraboGUI -y

# 3. Generate _version file
python setup.py --version

# 4. Generate meta.yaml recipe information
cd ./conda-recipe/
python -m cogapp -o meta.yaml ./meta_base.yaml

echo "**********Building KaraboGui with the following Recipe**********"
cat ./meta.yaml

# 6. Build the package
conda build ./ -c http://exflserv05.desy.de/karabo/channel \
               -c conda-forge \
               -c defaults \
               --no-anaconda-upload