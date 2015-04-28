#!/bin/bash

# AUTHOR: Mayank Kumar <mayank.kumar@xfel.eu>
# CREATED: April 28, 2015

# execute parallel ssh on all remote machines to stop all device servers

CONFIG_FILE=config.ini
DOWNLOAD_PARENT_DIR=$( grep download_parent_dir ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
HOSTS_FILENAME=$( grep hosts_filename ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
STOPALLDS_SCRIPT_FILE=$( grep stop_all_ds_script ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')

# run the installation and download script
pssh -h ${HOSTS_FILENAME} -t 600 -i -v -o . -e . -x "cd ${DOWNLOAD_PARENT_DIR} 2>/dev/null; bash" -I<"${STOPALLDS_SCRIPT_FILE}"
wait

exit 0