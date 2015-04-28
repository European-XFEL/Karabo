#!/bin/bash

# this script sets up the device servers on the remote machines.

# AUTHOR: Mayank Kumar <mayank.kumar@xfel.eu>
# CREATED: Apr 28, 2015

echo "connection opened: (start all device servers script)"

os_name=$(lsb_release -is)
os_version=$(lsb_release -rs)
if [ "${os_name}" = "Scientific" ]; then os_version=${os_version%%\.*}; fi
os_arch=$(uname -m)

CONFIG_FILE=config.iniSIMPLIFIED_DATA_FILE=$( grep simplified_data_file ${CONFIG_FILE} | cut -d '=' -f2 | tr -d ' ')
INFO_SUFFIX=$( grep info_suffix ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
host=$( hostname -f)
HOST_INFO_FILE=".${host}_${INFO_SUFFIX}"
INSTALLATION_NAME=$( grep installation_name ${HOST_INFO_FILE} | cut -d'=' -f2)
RUN_PREFIX=$( grep run_prefix ${HOST_INFO_FILE} | cut -d'=' -f2)
DEVICE_SERVER_IDS_LIST=$( grep ds_ids ${HOST_INFO_FILE} | cut -d'=' -f2)

IFS=',' read -a ds_ids_array <<< "${DEVICE_SERVER_IDS_LIST}"
counter=0
for one_ds_id in ${ds_ids_array[@]}
    do
        :
        start_script_dir="${RUN_PREFIX}/${INSTALLATION_NAME}/${one_ds_id}"
        cd ${start_script_dir}

        # find the start script
        start_script=$( find . -name 'start*' )

        # execute start script
        ${start_script}

    done

echo "connection closed: (start all device servers script)"

exit 0
