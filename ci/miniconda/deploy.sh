#!/usr/bin/env bash

set -e
set -o pipefail

# This script is responsible for deploying the generated package and mirror

CONDA_CHANNEL_PATH=/var/www/html/karabo/channel
PLATFORM=linux-64
REMOTE_SERVER=xkarabo@exflserv05
REMOTE_CHANNEL_PATH=${REMOTE_SERVER}:${CONDA_CHANNEL_PATH}/

# Get the package
KARABOGUI_PKG=$(conda info --root)/conda-bld/${PLATFORM}/karabogui-${CI_COMMIT_REF_NAME}*.tar.bz2

. ci/utils/enable_internet.sh

# Deploy the package
rsync --exclude \".git\" --rsh="sshpass -p \"${XKARABO_PWD}\" ssh -o StrictHostKeyChecking=no -l xkarabo" \
                  --progress ${KARABOGUI_PKG} ${REMOTE_CHANNEL_PATH}/${PLATFORM}/

# Deploy the mirror if exists
if [ "$(ls -A /tmp/mirror/)" ]; then
    rsync -r --exclude \".git\" --rsh="sshpass -p \"${XKARABO_PWD}\" ssh -o StrictHostKeyChecking=no -l xkarabo" \
                         --progress /tmp/mirror/* ${REMOTE_CHANNEL_PATH}/mirror/
fi

# the remote host should have a conda installation with conda-build available in the home folder

# Rebuild channel indexes
sshpass -p ${XKARABO_PWD} ssh -o StrictHostKeyChecking=no ${REMOTE_SERVER} "source ~/miniconda3/bin/activate; cd ${CONDA_CHANNEL_PATH}; conda index .;"

. ci/utils/disable_internet.sh