#!/bin/bash

# this script sets up the device servers on the remote machines.

# AUTHOR: Mayank Kumar <mayank.kumar@xfel.eu>
# CREATED: Dec 08, 2014

echo "connection opened: (setup device servers bash script)"

CONFIG_FILE=config.ini
DOWNLOAD_PARENT_DIR=$( grep download_parent_dir ${CONFIG_FILE} | cut -d '=' -f2 | tr -d ' ')
INSTALLED_PKGS_FILE=$( grep installed_pkgs_file ${CONFIG_FILE} | cut -d '=' -f2 | tr -d ' ')
SIMPLIFIED_DATA_FILE=$( grep simplified_data_file ${CONFIG_FILE} | cut -d '=' -f2 | tr -d ' ')
DEVICE_SERVER_IDS_FILE=$( grep device_server_ids_file ${CONFIG_FILE} | cut -d '=' -f2 | tr -d ' ')
INFO_SUFFIX=$( grep info_suffix ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
host=$( hostname -f)
HOST_INFO_FILE=".${host}_${INFO_SUFFIX}"
INSTALLATION_NAME=$( grep installation_name ${HOST_INFO_FILE} | cut -d'=' -f2)
BROKER_HOSTS=$( grep broker_hosts ${HOST_INFO_FILE} | cut -d'=' -f2)
RUN_PREFIX=$( grep run_prefix ${HOST_INFO_FILE} | cut -d'=' -f2)
INSTALL_PREFIX=$( grep install_prefix ${HOST_INFO_FILE} | cut -d'=' -f2)
KARABO_USER=$( grep karabo_user ${HOST_INFO_FILE} | cut -d'=' -f2)
DEVICE_SERVER_IDS_LIST=$( grep ds_ids ${HOST_INFO_FILE} | cut -d'=' -f2)
KARABO_RUN=$( grep karaboRun ${CONFIG_FILE} | cut -d'=' -f2)

BROKER_HOSTNAME=$(echo ${BROKER_HOSTS} | cut -d':' -f1 )
BROKER_PORT=$(echo ${BROKER_HOSTS} | cut -d':' -f2)

###### copy bin directory from karaboRun to inside run directory
ANY_KARABO=$( grep karabo_fws ${HOST_INFO_FILE} | cut -d'=' -f2 | cut -d',' -f1)
if [[ ! -d ${RUN_PREFIX}/${INSTALLATION_NAME}/bin ]]; then mkdir ${RUN_PREFIX}/${INSTALLATION_NAME}/bin; fi
cp ${INSTALL_PREFIX}/${ANY_KARABO}/${KARABO_RUN}/bin/* ${RUN_PREFIX}/${INSTALLATION_NAME}/bin/

IFS=',' read -a ds_ids_array <<< "${DEVICE_SERVER_IDS_LIST}"
for one_ds_id in ${ds_ids_array[@]}
    do
        :
        one_ds_id_dir="${RUN_PREFIX}/${INSTALLATION_NAME}/${one_ds_id}"
        if [[ ! -d ${one_ds_id_dir} ]]; then mkdir -p ${one_ds_id_dir}; fi

        one_ds_plugins_dir="${one_ds_id_dir}/plugins"
        if [[ ! -d ${one_ds_plugins_dir} ]]; then mkdir ${one_ds_plugins_dir}; fi

        serverid_xml_file="${one_ds_id_dir}/serverId.xml"
        if [[ ! -e ${serverid_xml_file} ]]; then touch ${serverid_xml_file}; fi
        echo ${one_ds_id} > ${serverid_xml_file}

        ####### clean all old links, if any
        if [ "$(ls -A ${one_ds_plugins_dir})" ]; then find ${one_ds_plugins_dir} -maxdepth 1 -type l -exec rm -f {} \;; fi #should be two semi-colons, it is not a typo!!


        DS_ID_INFO_FILE=".${host}_${one_ds_id}_${INFO_SUFFIX}"
        PLUGINS_LIST=$(grep plugins ${DS_ID_INFO_FILE} | cut -d'=' -f2)
        IFS=',' read -a plugins_array <<< "${PLUGINS_LIST}"
        for one_plugin in ${plugins_array[@]}
            do
                :
                karabo_fw_version=$(echo ${one_plugin} | cut -d'-' -f3)
                karabo_fw="karabo-${karabo_fw_version}"
                ( cd ${one_ds_plugins_dir}; find ${INSTALL_PREFIX}/${karabo_fw}/plugins/${one_plugin} -maxdepth 1 -type f \( -name \*.so -or \( -not -name main.py -name \*.py -exec grep -q 'class.*(.*Python.*Device' {} \;  \) \) -exec ln -sf {} \; )
            done

        python_found=$(find ${one_ds_plugins_dir} -name \*.py)
        cpp_found=$(find ${one_ds_plugins_dir} -name \*.so)
        server_type=""
        if [[ -n ${python_found} && -z ${cpp_found} ]]; then
            server_type="python"
        elif [[ -z ${python_found} && -n ${cpp_found} ]]; then
            server_type=""
        elif [[ -n ${python_found} && -n ${cpp_found} ]]; then
            echo "mixing python and cpp plugins in the same device server ${one_ds_id} is not permitted. Exiting."
            exit 2
        else
            echo "unable to determine type of device server (python or cpp). No links created and exiting."
            exit 2
        fi

        # create startup script
        cat > ${one_ds_id_dir}/start${server_type^}DeviceServer <<End-of-file
#!/bin/bash
#
# This file was automatically generated. Do not edit.
#

export KARABO_BROKER_HOST=${BROKER_HOSTNAME}
export KARABO_BROKER_PORT=${BROKER_PORT}
export KARABO_BROKER_TOPIC=${INSTALLATION_NAME}
KARABO=${INSTALL_PREFIX}/${karabo_fw}
SERVER_ID=${one_ds_id}
ARGS="DeviceServer.serverId=${one_ds_id}"

failed=0
failedmod10=0
starttime=\`date +%s\`

until \${KARABO}/bin/karabo-${server_type}deviceserver \${ARGS} "\$@"; do
    ret=\$?
    failed=\$((\$failed+1))
    failedmod10=\$((\$failed%10))
    if [[ \$failedmod10 -eq 0 ]]; then
       currenttime=\`date +%s\`
       ttime=\$((\$currenttime-\$starttime))
       if [[ \$ttime -lt 20 ]]; then
          echo "Too many respawns, exiting..."
          exit 1
       fi
    fi
    echo "\${SERVER_ID} crashed with exit code \$ret. Respawning.."
    sleep 2
    if [[ \$failedmod10 -eq 0 ]]; then
        starttime=\`date +%s\`
    fi
done

End-of-file
        chmod +x ${one_ds_id_dir}/start${server_type^}DeviceServer
    done

chown  -R ${KARABO_USER}:exfel "${RUN_PREFIX}"/"${INSTALLATION_NAME}"

echo "connection closed: (setup device servers bash script)"

exit 0
