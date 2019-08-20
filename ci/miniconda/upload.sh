#!/usr/bin/env bash

CONDA_CHANNEL_PATH=/var/www/html/karabo/channel
PLATFORM=linux-64

# This script is responsible for building and uploading the karabogui conda
# package to our channel. This is done whenever a new tag is generated

set -e
set -o pipefail

cd ./src/pythonGui/

PKG_FOLDER=$(conda info --root)/conda-bld/

# Upload our conda package
apt-get install -y sshpass rsync
sshpass -V
rsync --exclude ".git" --rsh="sshpass -p "${XDATA_PWD}" \
    ssh -o StrictHostKeyChecking=no -l xdata" \
    --progress ${PKG_FOLDER}/${PLATFORM}/karabogui-${CI_COMMIT_REF_NAME}*.tar.bz2 \
    ${SSH_USER_HOST}:${CONDA_CHANNEL_PATH}/${PLATFORM}

# Sync our mirror channel
rsync -r --exclude ".git" --rsh="sshpass -p "${XDATA_PWD}" \
    ssh -o StrictHostKeyChecking=no -l xdata" \
    --progress ~/mirror/ \
    ${SSH_USER_HOST}:${CONDA_CHANNEL_PATH}/mirror

# Rebuild channel index on remote for all channels
sshpass -p "${XDATA_PWD}" ssh -o StrictHostKeyChecking=no xdata@exflserv05 bash "~/scripts/rebuild_channel_index.sh"