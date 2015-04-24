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
KARABO_RUN=$( grep karaboRun ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
host=$( hostname -f)
HOST_INFO_FILE=".${host}_${INFO_SUFFIX}"
INSTALLATION_NAME=$( grep installation_name ${HOST_INFO_FILE} | cut -d'=' -f2)
BROKER_HOSTS=$( grep broker_hosts ${HOST_INFO_FILE} | cut -d'=' -f2)
RUN_PREFIX=$( grep run_prefix ${HOST_INFO_FILE} | cut -d'=' -f2)
INSTALL_PREFIX=$( grep install_prefix ${HOST_INFO_FILE} | cut -d'=' -f2)
KARABO_USER=$( grep karabo_user ${HOST_INFO_FILE} | cut -d'=' -f2)
DEVICE_SERVER_IDS_LIST=$( grep ds_ids ${HOST_INFO_FILE} | cut -d'=' -f2)

###### copy all scripts in bin directory from karaboRun to inside run directory
ANY_KARABO=$( grep karabo_fws ${HOST_INFO_FILE} | cut -d'=' -f2 | cut -d',' -f1)
if [[ ! -d ${RUN_PREFIX}/${INSTALLATION_NAME}/bin ]]; then mkdir ${RUN_PREFIX}/${INSTALLATION_NAME}/bin; fi
cp ${INSTALL_PREFIX}/${ANY_KARABO}/${KARABO_RUN}/bin/. ${RUN_PREFIX}/${INSTALLATION_NAME}/bin -R

# adjust allInfo file
cat > ${RUN_PREFIX}/${INSTALLATION_NAME}/bin/allInfo <<End-of-file

##########################################################################################
# allInfo contains configuration definitions used to manage master, single or multi device
# server, gui and other applications used in a localhost device test system.
#
# management scripts:
#   1. allStart - to start karabo processes defined by the ALLINFO environmental
#                 allStart has a single optional parameter "-nox" which controls whether
#                 the processes are started as individual xterms or accumulated as background jobs
#   2. allStop  - to stop started processes
#   3. allCheck - to check the definitions used by test system scripts
#                 this script is the only check made of the definitions used
#   5. allRestart - to restart any "missing" applicationsd
#                 a candidate to a missing application is the gui which was closed at
#                 the end of the last session with all other applications left running.
#
# author: CY 18.12.2012, largely modified: BH 09.04.2015


##################
#  Broker Hosts  #
##################

# This setting is the default, it will try the XFEL broker first and failover to a local if unsuccessful
export KARABO_BROKER_HOSTS=${BROKER_HOSTS}


#################
# Broker Topic  #
#################

# By default your username will become your topic name

# You may want to fix it using:

#export KARABO_BROKER_TOPIC=xbeam


##################################
# Kill Signal (used by allStop)  #
##################################

KARABO_KILL_SIGNAL=9


################
# Applications #
################

# Component definitions, currently 6 tokens per process
# 1. relative path of the starter filename (including the starter)
# 2. blank separated parameters to starter
# 3. delay, in seconds, before starting next process
# 4. number of scroll lines of xterm
# 5. if "yes" use the x11 display specified in XUSE environmental. To start applications on the
#    control host define XUSE to the main console ($ export XUSE=":0") and, provided your login account
#    is identical to the console's owner account, all applications with 9="yes" will be displayed there.
#    Applications started in this way will remain alive when the session is closed.
# Note: additional servers can be added by duplicating a server line and performing
# the necessary directory [definition 2] renaming (servers have unique directories)
# and providing a unique ALLINFO array index

KARABO_SERVERS="../"

End-of-file

IFS=',' read -a ds_ids_array <<< "${DEVICE_SERVER_IDS_LIST}"
counter=0
for one_ds_id in ${ds_ids_array[@]}
    do
        :
        one_ds_id_dir="${RUN_PREFIX}/${INSTALLATION_NAME}/${one_ds_id}"
        if [[ ! -d ${one_ds_id_dir} ]]; then mkdir -p ${one_ds_id_dir}; fi

        # create serverId.xml
        serverid_xml_file="${one_ds_id_dir}/serverId.xml"
        if [[ ! -e ${serverid_xml_file} ]]; then touch ${serverid_xml_file}; fi
        echo ${one_ds_id} > ${serverid_xml_file}

        # if server is dataLoggerServer, then do some things
        # elif its guiServer do something
        # elif its macroServer do something
        # else do something
        if [[ ${one_ds_id} = "dataLoggerServer" ]]; then
            echo "dataLoggerServer found"

            DS_ID_INFO_FILE=".${host}_${one_ds_id}_${INFO_SUFFIX}"
            KARABO_FW=$(grep karabo_fw ${DS_ID_INFO_FILE} | cut -d'=' -f2)

            #create autoload.xml
            cat > ${one_ds_id_dir}/autoload.xml <<End-of-file
<?xml version="1.0"?>
<DeviceServer>
  <autoStart>
    <KRB_Item>
      <DataLoggerManager>
      </DataLoggerManager>
    </KRB_Item>
  </autoStart>
  <scanPlugins KRB_Type="STRING">false</scanPlugins>
  <serverId KRB_Type="STRING">Karabo_DataLoggerServer</serverId>
  <visibility>4</visibility>
  <Logger><priority>INFO</priority></Logger>
</DeviceServer>
End-of-file
            #create startup script
            cat > ${one_ds_id_dir}/startCppServer <<End-of-file
#!/bin/bash
#
# This file was automatically generated. Do not edit.
#

KARABO=${INSTALL_PREFIX}/${KARABO_FW}

# Make sure the script runs in this directory
scriptDir=$(dirname `[[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"`)
cd \${scriptDir}

if [ $? -ne 0 ]; then
    echo "ERROR Could not change directory to \${scriptDir}"
    exit 1;
fi

# Make sure Karabo is installed
if [ -z \${KARABO} ]; then
    if [ -e $HOME/.karabo/karaboFramework ]; then
        KARABO=$(cat $HOME/.karabo/karaboFramework)
    else
      echo "ERROR Could not find karaboFramework. Make sure you have installed the karaboFramework."
      exit 1
    fi
fi

until \${KARABO}/bin/karabo-deviceserver "$@"; do
    echo "ERROR Karabo server went down with signal $?, respawning..."
    sleep 5
done

End-of-file
            chmod +x ${one_ds_id_dir}/startCppServer
            echo "ALLINFO[${counter}]=\${KARABO_SERVERS}/dataLoggerServer/startCppServer::0:100:yes" >> ${RUN_PREFIX}/${INSTALLATION_NAME}/bin/allInfo

        elif [[ ${one_ds_id} = "guiServer" ]]; then
            echo "guiServer found"
        elif [[ ${one_ds_id} = "macroServer" ]]; then
            echo "macroServer found"
        else
            echo "some other device server found"
            # create autoload.xml
            cat > ${one_ds_id_dir}/autoload.xml <<End-of-file
<?xml version="1.0"?>
<DeviceServer>
  <Logger>
    <priority>INFO</priority>
  </Logger>
</DeviceServer>
End-of-file
            # setup device server
            one_ds_plugins_dir="${one_ds_id_dir}/plugins"
            if [[ ! -d ${one_ds_plugins_dir} ]]; then mkdir ${one_ds_plugins_dir}; fi

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
            device_server_name=""
            special_argument=""
            if [[ -n ${python_found} && -z ${cpp_found} ]]; then
                server_type="python"
                device_server_name="PythonServerApi1"
                special_argument="DeviceServer"
            elif [[ -z ${python_found} && -n ${cpp_found} ]]; then
                server_type=""
                device_server_name="CppServer"
                special_argument=""
            elif [[ -n ${python_found} && -n ${cpp_found} ]]; then
                echo "mixing python and cpp plugins in the same device server ${one_ds_id} is not permitted. Exiting."
                exit 2
            else
                echo "unable to determine type of device server (python or cpp). No links created and exiting."
                exit 2
            fi

            configuration=$(grep configuration ${DS_ID_INFO_FILE} | cut -d'=' -f2)

            # create startup script
            cat > ${one_ds_id_dir}/start${device_server_name} <<End-of-file
#!/bin/bash
#
# This file was automatically generated. Do not edit.
#

export KARABO_BROKER_HOSTS=${BROKER_HOSTS}
export KARABO_BROKER_TOPIC=${INSTALLATION_NAME}
KARABO=${INSTALL_PREFIX}/${karabo_fw}
SERVER_ID=${one_ds_id}
# should take config from json file
ARGS=${configuration}

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
            echo "ALLINFO[${counter}]=\${KARABO_SERVERS}/${one_ds_id}/start${device_server_name}:${special_argument}:0:100:yes" >> ${RUN_PREFIX}/${INSTALLATION_NAME}/bin/allInfo
        fi
        counter=$((counter+1))
    done

chown  -R ${KARABO_USER}:exfel "${RUN_PREFIX}"/"${INSTALLATION_NAME}"

echo "connection closed: (setup device servers bash script)"

exit 0
