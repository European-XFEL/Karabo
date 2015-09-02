#!/bin/bash

# AUTHOR: Mayank Kumar <mayank.kumar@xfel.eu>
# CREATED: Feb 19, 2015

# download karabo framework, plugins
# install karabo framework, plugins
# download dependencies
# install dependencies

echo "connection opened: (download and install bash script)"

os_name=$(lsb_release -is)
os_version=$(lsb_release -rs)
if [ "${os_name}" = "Scientific" -o "${os_name}" = "CentOS" ]; then os_version=${os_version%%\.*}; fi
os_arch=$(uname -m)

CONFIG_FILE=config.ini
DOWNLOADED_KARABO_FILE=$( grep downloaded_karabo_file ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
DOWNLOADED_PKGS_FILE=$( grep downloaded_pkgs_file ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
DOWNLOADED_DEPS_FILE=$( grep downloaded_deps_file ${CONFIG_FILE}| cut -d'=' -f2 | tr -d ' ')
INSTALLED_KARABO_FILE=$( grep installed_karabo_file ${CONFIG_FILE} | cut -d '=' -f2 | tr -d ' ')
INSTALLED_PKGS_FILE=$( grep installed_pkgs_file ${CONFIG_FILE} | cut -d '=' -f2 | tr -d ' ')
INSTALLED_DEPS_FILE=$( grep installed_deps_file ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
REPOS_FILENAME=$( grep repos_filename ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
INFO_SUFFIX=$( grep info_suffix ${CONFIG_FILE} | cut -d'=' -f2 | tr -d ' ')
host=$( hostname -f)
HOST_INFO_FILE=".${host}_${INFO_SUFFIX}"
INSTALL_PREFIX=$( grep install_prefix ${HOST_INFO_FILE} | cut -d'=' -f2)
KARABO_FRAMEWORKS_LIST=$( grep karabo_fws ${HOST_INFO_FILE} | cut -d'=' -f2)
PLUGINS_LIST=$( grep plugins ${HOST_INFO_FILE} | cut -d'=' -f2)

#################### get repos
# TODO make it accept an array of repos
fw_repo=""
plugins_repo=""
deps_repo=""

if [[ ! -e "${REPOS_FILENAME}" ]]; then echo "repos file not found"; exit 1; fi
if [[ ! -e "${HOST_INFO_FILE}" ]]; then echo "repos file not found"; exit 1; fi

while read repos_line
do
    if [[ ${repos_line} == *karaboFramework* ]]; then fw_repo=$(echo ${repos_line} | cut -d'=' -f2); fi
    if [[ ${repos_line} == *karaboPackages* ]]; then plugins_repo=$(echo ${repos_line} | cut -d'=' -f2); fi
    if [[ ${repos_line} == *karaboDependencies* ]]; then deps_repo=$(echo ${repos_line} | cut -d'=' -f2); fi
done <${REPOS_FILENAME}

######### get package-file function
function getFromRepo () {

file=$1
repoList=$2
gotFile=0
OLDIFS=${IFS}
IFS=","
for repo in ${repoList}; do
    proto=${repo%%:*}
    case ${proto} in
        http)
            echo wget ${repo}/${file}
            wget ${repo}/${file}
            if [ $? -eq 0 ]; then
                gotFile=1
                echo "got file from http"
                break
            fi
            ;;
        ftp)
            echo wget ${repo}/${file}
            wget ${repo}/${file}
            if [ $? -eq 0 ]; then
                gotFile=1
                echo "got file from ftp"
                break
            fi
            ;;
        file)
            #echo curl -f -s -O ${repo}/${file}
            #curl -f -s -O ${repo}/${file}
            echo wget ${repo}/${file}
            wget ${repo}/${file}
            if [ $? -eq 0 ]; then
                gotFile=1
                echo "got file from file/local machine"
                break
            fi
            ;;
        *)
            echo "${repo} not supported"
    esac
if [ ${gotFile} -eq 0 ]; then
    echo "file not found"
    return 1
fi

done
IFS=${OLDIFS}
}

####### check if file exists on url or not
function check_url () {

file=$1
repoList=$2
OLDIFS=${IFS}
IFS=","
fileExists=0

for repo in ${repoList}; do
    proto=${repo%%:*}
    case ${proto} in
        http)
            wget -q ${repo}/${file}
            if [ $? -eq 0 ]; then
                fileExists=1
                echo "file found on http"
                break
            fi
            ;;
        ftp)
            wget -q ${repo}/${file}
            if [ $? -eq 0 ]; then
                fileExists=1
                echo "file found on ftp"
                break
            fi
            ;;
        file)
            wget -q ${repo}/${file}
            if [ $? -eq 0 ]; then
                fileExists=1
                echo "file found on file/local machine"
                break
            fi
            ;;
        *)
            echo "${repo} not supported"
    esac
if [ ${gotFile} -eq 0 ]; then
    echo "file not found anywhere. exiting."
    exit
fi

done
IFS=${OLDIFS}
}

####### create files if they don't exist
if [[ ! -e "${DOWNLOADED_KARABO_FILE}" ]]; then touch ${DOWNLOADED_KARABO_FILE}; fi
if [[ ! -e "${DOWNLOADED_PKGS_FILE}" ]]; then touch ${DOWNLOADED_PKGS_FILE}; fi
if [[ ! -e "${INSTALLED_KARABO_FILE}" ]]; then touch ${INSTALLED_KARABO_FILE}; fi
if [[ ! -e "${INSTALLED_PKGS_FILE}" ]]; then touch ${INSTALLED_PKGS_FILE}; fi
if [[ ! -e "${DOWNLOADED_DEPS_FILE}" ]]; then touch ${DOWNLOADED_DEPS_FILE}; fi
if [[ ! -e "${INSTALLED_DEPS_FILE}" ]]; then touch ${INSTALLED_DEPS_FILE}; fi

##################### download karabo framework
IFS=',' read -a karabo_fw_array <<< "${KARABO_FRAMEWORKS_LIST}"
for karabo_fw in ${karabo_fw_array[@]}
    do
        :
        karabo_fw_version=$(echo ${karabo_fw} | cut -d'-' -f2)
        karabo_fw_fullname="${karabo_fw}-Release-${os_name}-${os_version}-${os_arch}.sh"
        if ! grep -q ${karabo_fw_fullname} ${DOWNLOADED_KARABO_FILE} && ! grep -q ${karabo_fw_fullname} ${INSTALLED_KARABO_FILE}
        then
            echo "downloading: ${karabo_fw_fullname} .."
            karabo_fw_url="${karabo_fw_version}/${karabo_fw_fullname}"
            check_url ${karabo_fw_url} ${fw_repo}
            getFromRepo ${karabo_fw_url} ${fw_repo}
            wait
            echo "${karabo_fw_fullname}" >> ${DOWNLOADED_KARABO_FILE}
        else
            echo "${karabo_fw_fullname} already downloaded/installed."
        fi
    done

##################### download karabo plugins
IFS=',' read -a plugins_array <<< "${PLUGINS_LIST}"
for one_plugin in ${plugins_array[@]}
    do
        :
        plugin_fullname="${one_plugin}-${os_name}-${os_version}-${os_arch}.sh"
        karabo_fw_version=$(echo ${one_plugin} | cut -d'-' -f3)
        karabo_fw="karabo-${karabo_fw_version}"
        if ! grep -q ${plugin_fullname} ${DOWNLOADED_PKGS_FILE} && ! grep -q ${plugin_fullname} ${INSTALLED_PKGS_FILE}
        then
            echo "downloading: ${plugin_fullname} .."
            one_plugin_url="${karabo_fw}/${plugin_fullname}"
            check_url ${one_plugin_url} ${plugins_repo}
            getFromRepo ${one_plugin_url} ${plugins_repo}
            wait
            echo "${plugin_fullname}" >> ${DOWNLOADED_PKGS_FILE}
        else
            echo "${plugin_fullname} already downloaded/installed."
        fi
    done

################### install karabo framework
while read karabo_fw_fullname
do
    if ! grep -q ${karabo_fw_fullname} ${INSTALLED_KARABO_FILE} 2>/dev/null;
    then
        echo "installing: ${karabo_fw_fullname} .."
        chmod +x ${karabo_fw_fullname}
        bash ${karabo_fw_fullname} --prefix=${INSTALL_PREFIX}
        wait

        # add to list of installed karabo frameworks
        echo "${karabo_fw_fullname}" >> ${INSTALLED_KARABO_FILE}

        # create plugins dir in this karabo framework
        karabo_fw=$(echo ${karabo_fw_fullname} | cut -d'-' -f1,2)
        plugins_dir=${INSTALL_PREFIX}/${karabo_fw}/plugins
        if [[ ! -d ${plugins_dir} ]]; then mkdir ${plugins_dir}; fi
    fi
done <${DOWNLOADED_KARABO_FILE}

################# install plugin
while read plugin_fullname
do
    if ! grep -q ${plugin_fullname} ${INSTALLED_PKGS_FILE} 2>/dev/null;
    then
        echo "installing: ${plugin_fullname} .."
        chmod +x ${plugin_fullname}
        karabo_compiled_against=$(echo ${plugin_fullname} | cut -d'-' -f3)
        bash ${plugin_fullname} --prefix=${INSTALL_PREFIX}/karabo-${karabo_compiled_against}/plugins
        wait

        # add package name to list of installed packages
        echo "${plugin_fullname}" >> ${INSTALLED_PKGS_FILE}
    fi
done <${DOWNLOADED_PKGS_FILE}

################## download dependencies
while read one_plugin
do
    plugin_name_dir=$(echo ${one_plugin} | cut -d'-' -f1,2,3)
    karabo_fw_version=$(echo ${one_plugin} | cut -d'-' -f3)
    karabo_fw="karabo-${karabo_fw_version}"

    if [[ -e ${INSTALL_PREFIX}/${karabo_fw}/plugins/${plugin_name_dir}/DEPENDS ]]
    then
        echo "DEPENDS file found for plugin: ${plugin_name_dir}"
        while read depends_file_oneline
        do
            if [[ ${depends_file_oneline} == \#* ]]; then continue; fi
            if [[ ${depends_file_oneline} == '' ]]; then continue; fi
            dep_name=$(echo ${depends_file_oneline} | awk -F' +' '{ print $3}' )
            dep_version=$(echo ${depends_file_oneline} | awk -F' +' '{ print $4}' | awk -F'/' '{ print $2 }' )
            if [[ ${dep_version} == *"-"* ]]; then dep_version=$(echo ${dep_version} | awk -F'-' '{ print $1}'); fi
            dep_fullname="${dep_name}-${dep_version}-${karabo_fw_version}-${os_name}-${os_version}-${os_arch}.sh"
            if ! grep -q ${dep_fullname} ${DOWNLOADED_DEPS_FILE} && ! grep -q ${dep_fullname} ${INSTALLED_DEPS_FILE}
            then
                echo "downloading: ${dep_fullname} .."
                deps_url="${karabo_fw}/${dep_fullname}"
                check_url ${deps_url} ${deps_repo}
                getFromRepo ${deps_url} ${deps_repo}
                wait
                echo "${dep_fullname}" >> ${DOWNLOADED_DEPS_FILE}
            else
                echo "${dep_fullname} already downloaded/installed."
            fi

        done<${INSTALL_PREFIX}/${karabo_fw}/plugins/${plugin_name_dir}/DEPENDS
    fi
done <${INSTALLED_PKGS_FILE}

################### install dependencies
while read one_dep_name
do
    if ! grep -q ${one_dep_name} ${INSTALLED_DEPS_FILE};
    then
        echo "installing: ${one_dep_name}.."
        karabo_fw_version=$(echo ${one_dep_name} | cut -d'-' -f3)
        karabo_fw="karabo-${karabo_fw_version}"
        chmod +x ${one_dep_name}
        bash ${one_dep_name} --prefix=${INSTALL_PREFIX}/${karabo_fw}/extern
        wait
        echo "${one_dep_name}" >> ${INSTALLED_DEPS_FILE}
    fi
done <${DOWNLOADED_DEPS_FILE}

# in the end, remove all setup files, and empty the downloaded files list
find . -maxdepth 1 -type f -name "*.sh" -delete
rm ${DOWNLOADED_KARABO_FILE} ${DOWNLOADED_PKGS_FILE} ${DOWNLOADED_DEPS_FILE}

echo "connection closed: (download and install bash script)"

exit 0
