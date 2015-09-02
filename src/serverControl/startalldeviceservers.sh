#!/bin/bash

# this script sets up the device servers on the remote machines.

# AUTHOR: Mayank Kumar <mayank.kumar@xfel.eu>
# CREATED: Apr 28, 2015

echo "connection opened: (start all device servers script)"

os_name=$(lsb_release -is)
os_version=$(lsb_release -rs)
if [ "${os_name}" = "Scientific" -o "${os_name}" = "CentOS" ]; then os_version=${os_version%%\.*}; fi
os_arch=$(uname -m)

CONFIG_FILE=config.iniSIMPLIFIED_DATA_FILE=$( grep simplified_data_file ${CONFIG_FILE} | cut -d '=' -f2 | tr -d ' ')
INFO_SUFFIX=$( grep info_suffix ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
KARABO_RUN=$( grep karaboRun ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
host=$( hostname -f)
HOST_INFO_FILE=".${host}_${INFO_SUFFIX}"
INSTALLATION_NAME=$( grep installation_name ${HOST_INFO_FILE} | cut -d'=' -f2)
RUN_PREFIX=$( grep run_prefix ${HOST_INFO_FILE} | cut -d'=' -f2)

all_start_script="${RUN_PREFIX}/${INSTALLATION_NAME}/bin/allStart.sh"
bash ${all_start_script}

echo "connection closed: (start all device servers script)"

exit 0
