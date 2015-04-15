#!/bin/bash

# AUTHOR: Mayank Kumar <mayank.kumar@xfel.eu>
# CREATED: Feb 19, 2015

# execute parallel ssh on all remote machines

CONFIG_FILE=config.ini
DOWNLOAD_PARENT_DIR=$( grep download_parent_dir ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
HOSTS_FILENAME=$( grep hosts_filename ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
DOWNLOADANDINSTALL_SCRIPT_FILE=$( grep downloadandinstall_script_file ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
SETUP_SCRIPT_FILE=$( grep setup_device_servers_script_file ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')

# run the installation and download script
pssh -h ${HOSTS_FILENAME} -t 600 -i -v -o . -e . -x "cd ${DOWNLOAD_PARENT_DIR} 2>/dev/null; bash" -I<"${DOWNLOADANDINSTALL_SCRIPT_FILE}"
wait

# run the setup device servers script
pssh -h ${HOSTS_FILENAME} -t 600 -i -v -o . -e . -x "cd ${DOWNLOAD_PARENT_DIR} 2>/dev/null; bash" -I<"${SETUP_SCRIPT_FILE}"
wait

exit 0