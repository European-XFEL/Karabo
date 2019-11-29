#!/usr/bin/env bash

set -e
set -o pipefail

printAndRun() {
    typeset cmnd="$*"
    echo cmnd=$cmnd
    eval $cmnd
}

# This script is responsible for deploying the generated package and mirror

CONDA_CHANNEL_PATH=/var/www/html/karabo/channel
PLATFORM=linux-64
REMOTE_SERVER=xdata@exflserv05
REMOTE_CHANNEL_PATH=${REMOTE_SERVER}:${CONDA_CHANNEL_PATH}/

# Get the package
KARABOGUI_PKG=$(conda info --root)/conda-bld/${PLATFORM}/karabogui-${CI_COMMIT_REF_NAME}*.tar.bz2

. ci/utils/enable_internet.sh

# Deploy the package
printAndRun rsync --exclude \".git\" --rsh=\"sshpass -p \"${XDATA_PWD}\" ssh -o StrictHostKeyChecking=no -l xdata\" \
                  --progress ${KARABOGUI_PKG} ${REMOTE_CHANNEL_PATH}/${PLATFORM}/

# Deploy the mirror if exists
if [ "$(ls -A /tmp/mirror/)" ]; then
    printAndRun rsync -r --exclude \".git\" --rsh=\"sshpass -p \"${XDATA_PWD}\" ssh -o StrictHostKeyChecking=no -l xdata\" \
                         --progress /tmp/mirror/* ${REMOTE_CHANNEL_PATH}/mirror/
fi

# Rebuild channel indexes
printAndRun sshpass -p ${XDATA_PWD} ssh -o StrictHostKeyChecking=no ${REMOTE_SERVER} bash \"~/scripts/rebuild_channel_index.sh\"

. ci/utils/disable_internet.sh