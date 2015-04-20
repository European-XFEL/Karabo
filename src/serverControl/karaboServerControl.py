#!/usr/bin/env python3

import sys
import os
import argparse
import logging
import json
import pprint
import socket
import configparser
import urllib.request
import httplib2
import http.client

import jsonschema
import paramiko
import time

__author__ = "Mayank Kumar <mayank.kumar@xfel.eu>"
__date__ = "Nov 03, 2014"


################################################################################
####                              MAIN                                      ####
################################################################################

defaults_str = "defaults"
hosts_str = "hosts"
device_servers_str = "device_servers"
server_id_str = "server_id"
hostname_str = "hostname"
login_user_str = "login_user"
broker_str = "broker"
karabo_user_str = "karabo_user"
karabo_fw_version_str = "karabo_version"
install_prefix_str = "install_prefix"
run_prefix_str = "run_prefix"
plugins_str = "plugins"
name_str = "name"
version_str = "version"
port_str = "port"
installation_name_str = "installation_name"
karabo_fw_repositories_str = "karabo_repositories"
karabo_packages_repositories_str = "karabo_packages_repositories"
karabo_dependencies_repositories_str = "karabo_dependencies_repositories"

private_key_path = "/home/kumarm/.ssh/id_rsa"

json_as_dict = {}
installation_name = ""
broker_hostname = ""
broker_port = ""

config_file = os.path.expanduser(
    '~/PycharmProjects/karabo/src/serverControl/config.ini')
config_filename = "config.ini"
config = configparser.ConfigParser()
config.read(config_file)
download_parent_dir = config.get("DEFAULT", "download_parent_dir")
downloadandinstall_script_filename =\
    config.get("DEFAULT", "downloadandinstall_script_file")
setupdeviceservers_script_filename =\
    config.get("DEFAULT", "setup_device_servers_script_file")
ds_ids_filename = config.get("DEFAULT", "device_server_ids_file")
hosts_filename = config.get("DEFAULT", "hosts_filename")
repos_filename = config.get("DEFAULT", "repos_filename")
info_suffix = config.get("DEFAULT", "info_suffix")


def main():
    """
    main function

    """
    parser = argparse.ArgumentParser(
        prog='karabo-server-control',
        description='Karabo server control. Tool to manage Karabo installation')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('-v', '--verbose', action='count',
                       help='set verbosity level (-v, -vv)')
    group.add_argument('-q', '--quiet', action='store_true',
                       help='do not display anything')
    group.add_argument('-n', '--just-print', action='store_true',
                       help='simulate what the script would do on hosts')
    parser.add_argument('-H', '--host', action='store',
                        help='run the tool as only this host would'
                             ' be defined in configuration file')

    subparsers = parser.add_subparsers()

    # validate
    validate_parser = subparsers.add_parser('validate')
    validate_parser.add_argument('jsonfile',
                                 help='JSON file with configuration')
    validate_parser.set_defaults(func=validate)

    # install
    install_parser = subparsers.add_parser('install')
    install_parser.add_argument('jsonfile', help='JSON file with configuration')
    install_parser.set_defaults(func=install)

    # update
    update_parser = subparsers.add_parser('update')
    update_parser.add_argument('file_new',
                               help='JSON file with new configuration')
    update_parser.add_argument(
        'file_latest',
        help='JSON file with the latest applied configuration')
    update_parser.set_defaults(func=update)

    # uninstall
    uninstall_parser = subparsers.add_parser('uninstall')
    uninstall_parser.add_argument(
        'file_latest',
        help='JSON file with the latest applied configuration')
    uninstall_parser.set_defaults(func=uninstall)

    # check
    check_parser = subparsers.add_parser('check')
    check_parser.add_argument('jsonfile', help='JSON file with configuration')
    check_parser.set_defaults(func=check)

    # start
    start_parser = subparsers.add_parser('start')
    start_parser.set_defaults(func=start)

    # stop
    stop_parser = subparsers.add_parser('stop')
    stop_parser.set_defaults(func=stop)

    arguments_cli = parser.parse_args()

    # for now while development is going on, get all logs.
    # later when tool runs fine then set log_level to WARN
    #log_level = logging.WARN
    log_level = logging.DEBUG
    if arguments_cli.verbose == 1:
        log_level = logging.INFO
    if arguments_cli.verbose == 2:
        log_level = logging.DEBUG
    if arguments_cli.verbose == 3:
        log_level = logging.DEBUG

    if arguments_cli.quiet:
        log_level = logging.CRITICAL

    if arguments_cli.just_print:
        log_level = logging.INFO

    logging.basicConfig(format='%(message)s', level=log_level)

    logging.debug("INFO: Command line arguments: " + pprint.pformat(sys.argv))
    logging.debug("Parsed arguments: " + str(arguments_cli))

    return_code = arguments_cli.func(arguments_cli)
    exit(return_code)


def validate(args):
    """
    Validate if the configuration file has correct syntax and if it
    complies to the JSON schema.
    """
    return_code = validate_main(args)
    exit(return_code)


def install(args):
    """
    Install fresh configuration defined in the file.
    """
    return_code = install_main(args)
    exit(return_code)


def update(args):
    """
    Update configuration. The function requires two configurations:
    latest already applied to hosts and new one.
    The function figures out what has changed and only applies these changes.
    """
    return_code = update_main(args)
    exit(return_code)


def uninstall(args):
    """
    Uninstall configuration. The configuration file must contain the
    latest applied configuration.
    """
    return_code = uninstall_main(args)
    exit(return_code)


def check(args):
    """
    Check if the configuration defined in the file is applied to the hosts.
    """
    return_code = check_main(args)
    exit(return_code)


def start(args):
    """
    Start device server.
    """
    return_code = start_main(args)
    exit(return_code)


def stop(args):
    """
    Stop device servers.
    """
    return_code = stop_main(args)
    exit(return_code)


################################################################################
####                           VALIDATE                                     ####
################################################################################
def validate_main(arg_validate):
    """
    first entry into module_validate
    :param arg_validate:
    :return: integer
    """
    logging.debug("DEBUG: sub-command: validate " + arg_validate.jsonfile)

    # get all data structures
    json_filename = arg_validate.jsonfile
    global json_as_dict
    json_as_dict = load_json(json_filename)

    # validate json file's schema against standardized schema
    if not validate_schema_of_json_file():
        logging.error("ERROR: Configuration does not comply to the schema."
                      " Please check configuration and try again."
                      " Exiting now.")
        return False
    else:
        logging.info("INFO: Configuration complies to the standard schema..")

    # ensure json configuration is valid
    if not do_sanity_checks_on_json_file():
        logging.error("ERROR: Configuration is invalid. Exiting.")
        return False

    logging.info("INFO: Configuration is valid.")
    return True


def validate_schema_of_json_file():
    """
    validate schema of input json file against standard schema
    :return: true or false
    """

    # check if schema_version is present
    schema_version_str = "schema_version"
    if not schema_version_str in json_as_dict:
        logging.error("ERROR: schema version not defined in the JSON file."
                      " Please check the configuration file.")
        return False
    else:
        schema_version_value = json_as_dict[schema_version_str]
        if not isinstance(schema_version_value, int):
            logging.error("ERROR: Schema version must be an integer value."
                          "Exiting.")
            return False

    # check if schema_version is supported
    supported_schema = __SUPPORTED_SCHEMA_VERSIONS__
    if not schema_version_value in supported_schema:
        logging.error("ERROR: Schema version: " + str(schema_version_value) +
                      " not supported by this tool.")
        return False
    logging.debug("DEBUG: schema successfully retrieved..")

    # extract schema from file
    schema_as_dict = {}
    schema_as_dict = get_schema_as_dict(schema_version_value)
    if not schema_as_dict:
        logging.error("ERROR: cannot get schema. Exiting.")
        return False

    # check schema against standard library schema
    try:
        jsonschema.Draft4Validator.check_schema(schema_as_dict)
    except Exception as err:
        logging.error("ERROR: Error in Draft4Validator of jsonschema"
                      " while validating. Please check. Exiting.")
        logging.error(str(err))
        return False

    logging.debug("DEBUG: ====== Loaded schema ======")
    logging.debug(pprint.pformat(schema_as_dict))

    # validate schema against standard library schema
    logging.debug("DEBUG: validating JSON file...")
    try:
        validator_json = jsonschema.Draft4Validator(
            schema_as_dict, format_checker=jsonschema.FormatChecker())
        if not validator_json.is_valid(json_as_dict):
            allerrors = validator_json.iter_errors(json_as_dict)
            for one_error in sorted(allerrors, key=str):
                logging.error(pprint.pformat(one_error.message))
            return False
    except Exception as err2:
        logging.error(str(err2))
        return False

    return True


def do_sanity_checks_on_json_file():
    """
    ensure input json file passes all sanity checks (e.g. all hostnames
    unique, all hostnames resolvable, all plugins available etc.)
    :return:
    """
    # get all hostnames from json file
    if not get_all_hostnames():
        logging.error("ERROR: error in retrieving hostnames. Exiting.")
        return False
    else:
        logging.debug("DEBUG: successfully retrieved all hostnames..")

    # get all karabo versions from json file
    if not get_all_karabo_fw_versions():
        logging.error("ERROR: error in retrieving karabo versions. Exiting.")
        return False
    else:
        logging.debug("DEBUG: successfully retrieved all karabo fw versions..")

    # get all device server ids from json file
    if not get_all_device_server_ids():
        logging.error("ERROR: error in retrieving device server ids. Exiting.")
        return False
    else:
        logging.debug("DEBUG: successfully retrieved all device servers..")

    # ensure map (hostname -> device server ids) is created
    if not get_map_hostname_to_device_server_ids():
        logging.error("ERROR: cannot create map of hostname to "
                      "device server ids. Please check input JSON file / code."
                      " Exiting.")
        return False
    else:
        logging.debug("DEBUG: successfully created map of hostname to "
                      "device server ids..")

    # ensure map (device server id -> plugin long names) is created
    if not get_map_device_server_id_to_plugin_long_names():
        logging.error("ERROR: cannot create map of device server id to "
                      "plugin names. Please check input JSON file / code."
                      " Exiting.")
        return False
    else:
        logging.debug("DEBUG: successfully created map of device server id to "
                      "plugin names..")

    # ensure map (hostname -> plugin long names) is created
    if not get_map_hostname_to_plugin_long_names():
        logging.error("ERROR: cannot create map of hostname to "
                      "plugin names. Please check input JSON file / code."
                      " Exiting.")
        return False
    else:
        logging.debug("DEBUG: successfully created map of hostname to "
                      "plugin names..")

    # ensure map (hostname -> login user) is created
    if not get_map_hostname_to_login_user():
        logging.error("ERROR: cannot create map of hostname to "
                      "login user. Please check input JSON file / code."
                      "Exiting.")
        return False
    else:
        logging.debug("DEBUG: successfully created map of hostname to "
                      "login user..")

    # ensure map (hostname -> karabo user) is created
    if not get_map_hostname_to_karabo_user():
        logging.error("ERROR: cannot create map of hostname to "
                      "karabo user. Please check input JSON file / code."
                      "Exiting.")
        return False
    else:
        logging.debug("DEBUG: successfully created map of hostname to "
                      "karabo user..")

    # ensure map (hostname -> run prefix) is created
    if not get_map_hostname_to_run_prefix():
        logging.error("ERROR: cannot create map of hostname to "
                      "run prefix. Please check input JSON file / code."
                      "Exiting.")
        return False
    else:
        logging.debug("DEBUG: successfully created map of hostname to "
                      "run prefix..")

    # ensure map (hostname -> karabo fw versions) is created
    if not get_map_hostname_to_karabo_fw_versions():
        logging.error("ERROR: cannot create map of hostname to"
                      "karabo fw versions. Please check input JSON file / code."
                      "Exiting.")
        return False
    else:
        logging.debug("DEBUG: successfully created map of hostname to "
                      "karabo fw versions..")

    # ensure map (device server id -> karabo fw version) is created
    if not get_map_device_server_id_to_karabo_fw():
        logging.error("ERROR: cannot create map of device server id "
                      "to karabo fw version. Please check input JSON file "
                      "/ code. Exiting.")
    else:
        logging.debug("DEBUG: successfully created map of device server id "
                      "to karabo fw version..")

    # ensure all hostnames can be resolved
    for hostname in get_all_hostnames():
        if not is_hostname_resolvable(hostname):
            logging.error("ERROR: hostname ' + hostname + ' could not"
                          " be resolved")
            return False
        else:
            logging.debug("DEBUG: successfully resolved hostname:"
                          + hostname + "..")
    logging.debug("DEBUG: successfully resolved all hostnames..")

    # ensure broker hostname can be resolved
    broker_host_name = json_as_dict[broker_str][hostname_str]
    if not is_hostname_resolvable(broker_host_name):
        logging.error("ERROR: broker hostname could not be resolved. Exiting.")
        return False
    else:
        logging.debug("DEBUG: successfully resolved broker hostname..")

    # create a hosts file which is used for login to all hosts
    if not create_hosts_file():
        logging.error("ERROR: could not create hosts file from "
                      "json file. Exiting.")
        return False
    else:
        logging.debug("DEBUG: successfully created hosts file..")

    return True


def is_hostname_resolvable(hostname):
    """
    check if provided hostname can be resolved or not
    :param hostname:
    :return:
    """
    try:
        socket.gethostbyname(hostname)
        return True
    except socket.error:
        return False


def load_json(json_filename):
    """
    laod json file into memory.
    :param json_filename: name of json file.
    :return: contents of json file.
    """
    with open(json_filename) as jsonfile:
        return json.load(jsonfile)


################################################################################
####                           GET                                          ####
################################################################################
def get_schema_as_dict(schema_version):
    """
    extract schema from json file
    :param schema_version: schema version as in json file
    :return: schema of json as python dict
    """
    if schema_version in __SCHEMA__.keys():
        schema_as_string = __SCHEMA__[schema_version]
        schema_as_dict = {}

        # the function 'json-loads' converts string to dict
        try:
            schema_as_dict = json.loads(schema_as_string)
        except ValueError as value_error:
            logging.error("ERROR: Syntax error in schema definition."
                          " Contact the author of the tool.")
            logging.error("ERROR:" + str(value_error))
        return schema_as_dict
    else:
        logging.error("ERROR: cannot get schema version as string. Please check"
                      " input JSON file. Exiting.")
        return False


def get_all_hostnames():
    """
    extract all hostnames from json file
    :return: set containing all hostnames
    """
    logging.debug("DEBUG: getting complete list of hostnames...")
    all_hostnames_as_set = set()
    all_hosts_as_dict = json_as_dict[hosts_str]

    for one_host in all_hosts_as_dict:
        hostname = one_host[hostname_str]

        if hostname in all_hostnames_as_set:
            logging.error("ERROR: hostname " + str(hostname) +
                          " defined multiple times. Please correct JSON file"
                          " and try again. Exiting.")
            return set()
        else:
            all_hostnames_as_set.add(hostname)
            logging.debug("DEBUG: added host " + str(hostname) +
                          " to list of hostnames")

    logging.debug("DEBUG: complete list of hostnames: " +
                  str(all_hostnames_as_set))

    return all_hostnames_as_set


def get_all_device_server_ids():
    """
    extract all device server ids from json file
    :return: set containing all device server ids
    """
    all_device_server_ids_as_set = set()
    hosts_as_dict = json_as_dict[hosts_str]

    for one_host in hosts_as_dict:
        device_servers_as_dict = one_host[device_servers_str]
        for one_device_server in device_servers_as_dict:
            device_server_id = one_device_server[server_id_str]
            if device_server_id in all_device_server_ids_as_set:
                logging.error("ERROR: device server " + device_server_id +
                              " defined multiple times. Please correct"
                              " JSON file and try again. Exiting.")
                return set()
            else:
                all_device_server_ids_as_set.add(device_server_id)
                logging.debug("DEBUG: added server id " +
                              device_server_id +
                              " to list of device servers.")

    logging.debug("DEBUG: complete list of device server ids: " +
                  str(all_device_server_ids_as_set))

    return all_device_server_ids_as_set


def get_all_karabo_fw_versions():
    """

    :return:
    """
    all_karabo_versions_as_set = set()

    # first add the default value, if present
    default_karabo_version = json_as_dict[defaults_str][karabo_fw_version_str]
    all_karabo_versions_as_set.add(default_karabo_version)

    # then add all other values found inside the host
    hosts_as_dict = json_as_dict[hosts_str]
    for one_host in hosts_as_dict:
        if karabo_fw_version_str in one_host:
            all_karabo_versions_as_set.add(one_host[karabo_fw_version_str])

    return all_karabo_versions_as_set


def get_map_hostname_to_device_server_ids():
    """
    create map (hostname -> device server ids)
    device server ids are MULTIPLE per host.
    :return:
    """
    map_hostname_to_device_server_ids = {}

    all_hosts_as_dict = json_as_dict[hosts_str]

    for one_host in all_hosts_as_dict:
        one_hostname = one_host[hostname_str]
        all_device_servers_as_dict = one_host[device_servers_str]
        all_device_server_ids_as_set = set()

        for one_device_server in all_device_servers_as_dict:
            one_device_server_id = one_device_server[server_id_str]
            if not one_device_server_id in \
                    map_hostname_to_device_server_ids.values():
                all_device_server_ids_as_set.add(one_device_server_id)

        # add (one_hostname -> device server ids) to map
        map_hostname_to_device_server_ids[one_hostname] = \
            all_device_server_ids_as_set

    return map_hostname_to_device_server_ids


def get_map_device_server_id_to_karabo_fw():
    """
    create map (device server id -> karabo fw version)
    :return:
    """
    map_device_server_id_to_karabo_fw = {}

    hosts_as_dict = json_as_dict[hosts_str]

    default_karabo_version = json_as_dict[defaults_str][karabo_fw_version_str]

    for one_host in hosts_as_dict:
        device_servers_as_dict = one_host[device_servers_str]

        for one_ds in device_servers_as_dict:
            ds_id = one_ds[server_id_str]
            if karabo_fw_version_str in one_ds:
                karabo_fw = "karabo-" + one_ds[karabo_fw_version_str]
            else:
                karabo_fw = "karabo-" + default_karabo_version

            map_device_server_id_to_karabo_fw[ds_id] = karabo_fw

    return map_device_server_id_to_karabo_fw


def get_map_device_server_id_to_plugin_long_names():
    """
    create map (device server id -> set of plugins).
    note: plugins can be MULTIPLE per device server id.
    long name implies the following format:
    {pluginname}-{pluginversion}-{karabo_fw_version}
    :return:
    """
    map_device_server_id_to_plugin_long_names = {}

    default_karabo_version = json_as_dict[defaults_str][karabo_fw_version_str]

    hosts_as_dict = json_as_dict[hosts_str]
    for one_host in hosts_as_dict:
        device_servers_as_dict = one_host[device_servers_str]

        for one_device_server in device_servers_as_dict:
            device_server_id = one_device_server[server_id_str]
            all_plugin_long_names_as_set = set()

            if karabo_fw_version_str in one_device_server:
                karabo_fw_version = one_device_server[karabo_fw_version_str]
            else:
                karabo_fw_version = default_karabo_version

            for one_plugin in one_device_server[plugins_str]:
                plugin_name = one_plugin[name_str]
                plugin_version = one_plugin[version_str]

                plugin_long_name = "{}-{}-{}".format(plugin_name,
                                                     plugin_version,
                                                     karabo_fw_version)

                # if plugin not in set already, add it
                if not plugin_long_name in \
                        all_plugin_long_names_as_set:
                    all_plugin_long_names_as_set.add(plugin_long_name)
                    logging.debug("DEBUG: added plugin " +
                                  plugin_long_name + " to list of all "
                                  "plugins for device server " +
                                  device_server_id)
            # add (device server id -> plugins) to map
            map_device_server_id_to_plugin_long_names[device_server_id] = \
                all_plugin_long_names_as_set

    return map_device_server_id_to_plugin_long_names


def get_map_hostname_to_plugin_long_names():
    """
    map of one hostname to all plugins for this host.
    plugin long name has the format:
    {pluginname}-{pluginversion}-{karaboversion}
    mapping is one -> many for this map.
    :return:
    """
    map_hostname_to_plugin_long_names = {}

    default_karabo_version = json_as_dict[defaults_str][karabo_fw_version_str]

    hosts_as_dict = json_as_dict[hosts_str]
    for one_host in hosts_as_dict:
        one_hostname = one_host[hostname_str]
        device_servers_as_dict = one_host[device_servers_str]
        all_plugin_long_names_as_set = set()

        for one_device_server in device_servers_as_dict:

            if karabo_fw_version_str in one_device_server:
                karabo_fw_version = one_device_server[karabo_fw_version_str]
            else:
                karabo_fw_version = default_karabo_version

            for one_plugin in one_device_server[plugins_str]:
                plugin_name = one_plugin[name_str]
                plugin_version = one_plugin[version_str]

                plugin_long_name = "{}-{}-{}".format(plugin_name,
                                                     plugin_version,
                                                     karabo_fw_version)

                # if plugin not in set already, add it
                if not plugin_long_name in all_plugin_long_names_as_set:
                    all_plugin_long_names_as_set.add(plugin_long_name)

            # add (hostname -> plugins) to map
        map_hostname_to_plugin_long_names[one_hostname] = \
            all_plugin_long_names_as_set

    return map_hostname_to_plugin_long_names


def get_map_hostname_to_login_user():
    """
    create map (hostname -> login user)
    note: login user is ONCE per host.
    :return:
    """
    all_hosts_as_dict = json_as_dict[hosts_str]
    map_hostname_to_login_user = {}

    default_login_user = json_as_dict[defaults_str][login_user_str]

    for one_host in all_hosts_as_dict:
        one_hostname = one_host[hostname_str]

        if login_user_str in one_host:
            one_login_user = one_host[login_user_str]
            map_hostname_to_login_user[one_hostname] = \
                one_login_user
        else:
            one_login_user = default_login_user
            map_hostname_to_login_user[one_hostname] = \
                one_login_user

    return map_hostname_to_login_user


def get_map_hostname_to_run_prefix():
    """
    create map (hostname -> run prefix)
    note: run prefix is ONCE per host.
    :return:
    """
    all_hosts_as_dict = json_as_dict[hosts_str]
    map_hostname_to_run_prefix = {}

    default_run_prefix = json_as_dict[defaults_str][run_prefix_str]

    for one_host in all_hosts_as_dict:
        hostname = one_host[hostname_str]

        if run_prefix_str in one_host:
            map_hostname_to_run_prefix[hostname] = \
                one_host[run_prefix_str]
        else:
            map_hostname_to_run_prefix[hostname] = \
                default_run_prefix

    return map_hostname_to_run_prefix


def get_map_hostname_to_install_prefix():
    """
    create map (hostname -> install prefix)
    :return:
    """
    all_hosts_as_dict = json_as_dict[hosts_str]
    map_hostname_to_install_prefix = {}

    default_install_prefix = json_as_dict[defaults_str][install_prefix_str]

    for one_host in all_hosts_as_dict:
        hostname = one_host[hostname_str]

        if install_prefix_str in one_host:
            map_hostname_to_install_prefix[hostname] = \
                one_host[install_prefix_str]
        else:
            map_hostname_to_install_prefix[hostname] = \
                default_install_prefix

    return map_hostname_to_install_prefix


def get_map_hostname_to_karabo_user():
    """
    create map (hostname -> karabo user)
    note: karabo user is ONCE per host.
    :return:
    """
    all_hosts_as_dict = json_as_dict[hosts_str]
    map_hostname_to_karabo_user = {}

    default_karabo_user = json_as_dict[defaults_str][karabo_user_str]

    for one_host in all_hosts_as_dict:
        hostname = one_host[hostname_str]

        if run_prefix_str in one_host:
            map_hostname_to_karabo_user[hostname] = \
                one_host[karabo_user_str]
        else:
            map_hostname_to_karabo_user[hostname] = \
                default_karabo_user

    return map_hostname_to_karabo_user


def get_map_hostname_to_karabo_fw_versions():
    """
    create map (hostname -> karabo fw versions).
    first create set of karabo fw versions,
    then add to map (hostname -> karabo fw versions set).
    note: karabo fw versions can be MULTIPLE per host.
    :return:
    """
    all_hosts_as_dict = json_as_dict[hosts_str]
    map_hostname_to_karabo_fw_versions = {}

    default_karabo_fw_version = \
        json_as_dict[defaults_str][karabo_fw_version_str]

    for one_host in all_hosts_as_dict:
        hostname = one_host[hostname_str]

        karabo_fw_versions_as_set = set()

        for one_ds in one_host[device_servers_str]:
            if karabo_fw_version_str in one_ds:
                karabo_fw_versions_as_set.add(
                    "karabo-" + one_ds[karabo_fw_version_str])
            else:
                karabo_fw_versions_as_set.add(
                    "karabo-" + default_karabo_fw_version)
        # add to map now
        map_hostname_to_karabo_fw_versions[hostname] = karabo_fw_versions_as_set

    return map_hostname_to_karabo_fw_versions


def create_hosts_file():
    """

    :return:
    """

    #start fresh
    if os.path.isfile(hosts_filename):
        os.system("rm {}".format(hosts_filename))

    map1 = get_map_hostname_to_login_user()

    for key in map1:
        value = map1.get(key)

        # write to disk
        with open(hosts_filename, 'a') as file:
                file.write(value + "@" + key + "\n")

    return True


################################################################################
####                           INSTALL                                      ####
################################################################################
def install_main(args):
    """
    first entry into module_install
    :param args:
    """
    json_filename = args.jsonfile
    logging.debug("DEBUG: sub-command: install " + json_filename)

    #### validate input json file, like always
    if not validate_main(args):
        logging.error("ERROR: validation failed. Exiting.")
        return False

    #### download and install
    if not install_all():
        logging.error("ERROR: either installing of karabo framework / plugins"
                      "/dependencies failed or setting up the run environment"
                      "failed. Exiting.")
        return False

    return True


def install_all():
    """

    :rtype : true or false based on success or failure of function
    """
    karabo_fw_repository_as_list = json_as_dict[karabo_fw_repositories_str]
    karabo_fw_repository_as_csv = ",".join(karabo_fw_repository_as_list)

    karabo_packages_repository_as_list = \
        json_as_dict[karabo_packages_repositories_str]
    karabo_packages_repository_as_csv =\
        ",".join(karabo_packages_repository_as_list)

    karabo_deps_repository_as_list = \
        json_as_dict[karabo_dependencies_repositories_str]
    karabo_deps_repository_as_csv =\
        ",".join(karabo_deps_repository_as_list)

    # create file with list of repos
    if os.path.isfile(repos_filename):
        os.system("rm {}".format(repos_filename))
    with open(repos_filename, 'a') as file:
        file.write("framework=" + karabo_fw_repository_as_csv + "\n")
        file.write("plugins=" + karabo_packages_repository_as_csv + "\n")
        file.write("dependencies=" + karabo_deps_repository_as_csv + "\n")

    all_hosts_as_dict = json_as_dict[hosts_str]

    # loop over all hosts one by one and download the packages
    for one_host in all_hosts_as_dict:
        one_hostname = one_host[hostname_str]
        one_login_user = get_map_hostname_to_login_user()[one_hostname]

        # make ssh connection with host
        client = paramiko.SSHClient()
        private_key =\
            paramiko.RSAKey.from_private_key_file(private_key_path)
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        client.connect(one_hostname, username=one_login_user, pkey=private_key)
        sftp_client = client.open_sftp()

        # get parameters from machine
        # get name of OS (e.g. Ubuntu)
        os_name = exec_command_oneliner(client, "lsb_release -is")
        # get version of OS (e.g. 14.04)
        os_version = exec_command_oneliner(client, "lsb_release -rs")
        # for Scientific Linux, get only major release
        if os_name == "Scientific":
            os_version = str(os_version)
            os_version = os_version.split('.')[0]
            os_version = int(os_version)
        # either of the above cases, convert back to string
        os_version = str(os_version)
        # get architecture of OS (e.g. x86_64)
        os_arch = exec_command_oneliner(client, "uname -m")
        print(one_hostname, os_name, os_version, os_arch)

        # ensure destination dir exists
        cmd_create_download_parent_dir = "if [[ ! -d {0} ]];" \
                                         "then mkdir -p {0};" \
                                         "fi".\
                                         format(download_parent_dir)
        exec_command_oneliner(client, cmd_create_download_parent_dir)
        logging.debug("DEBUG: destination parent dir created/already exists..")

        device_server_ids_set = set()
        device_server_ids_set = \
            get_map_hostname_to_device_server_ids()[one_hostname]

        karabo_fws_set = set()
        karabo_fws_set =\
            get_map_hostname_to_karabo_fw_versions()[one_hostname]

        one_host_info_filename = "." + one_hostname + "_" + info_suffix
        if os.path.isfile(one_host_info_filename):
            os.system("rm {}".format(one_host_info_filename))

        one_install_prefix =\
            get_map_hostname_to_install_prefix()[one_hostname]
        one_run_prefix =\
            get_map_hostname_to_run_prefix()[one_hostname]
        one_karabo_user =\
            get_map_hostname_to_karabo_user()[one_hostname]
        ds_ids_list = list(device_server_ids_set)
        all_ds_ids_csv = ",".join(ds_ids_list)
        karabo_fws_list = list(karabo_fws_set)
        all_kf_csv = ",".join(karabo_fws_list)
        all_plugins_long_names_for_one_host_set = set()
        all_plugins_long_names_for_one_host_set =\
            get_map_hostname_to_plugin_long_names()[one_hostname]
        all_plugins_long_names_for_one_host_list =\
            list(all_plugins_long_names_for_one_host_set)
        all_plugins_long_names_for_one_host_csv =\
            ",".join(all_plugins_long_names_for_one_host_list)
        global installation_name
        installation_name = json_as_dict[installation_name_str]
        global broker_hostname
        broker_hostname = json_as_dict[broker_str][hostname_str]
        global broker_port
        broker_port = json_as_dict[broker_str][port_str]
        broker_port = str(broker_port)

        # write host info
        with open(one_host_info_filename, 'a') as file:
            file.write("installation_name=" + installation_name + "\n")
            file.write("broker_hostname=" + broker_hostname + "\n")
            file.write("broker_port=" + broker_port + "\n")
            file.write("install_prefix=" + one_install_prefix + "\n")
            file.write("run_prefix=" + one_run_prefix + "\n")
            file.write("karabo_user=" + one_karabo_user + "\n")
            file.write("ds_ids=" + all_ds_ids_csv + "\n")
            file.write("karabo_fws=" + all_kf_csv + "\n")
            file.write("plugins=" + all_plugins_long_names_for_one_host_csv +
                       "\n")

        for one_ds_id in device_server_ids_set:
            karabo_fw_version =\
                get_map_device_server_id_to_karabo_fw()[one_ds_id]

            one_host_one_ds_id_info_filename = "." + one_hostname + "_" + \
                one_ds_id + "_" + info_suffix
            if os.path.isfile(one_host_one_ds_id_info_filename):
                os.system("rm {}".format(one_host_one_ds_id_info_filename))

            # write host ds id info to file
            one_karabo_user = get_map_hostname_to_karabo_user()[one_hostname]

            all_plugins_long_names_for_one_ds_set = set()
            all_plugins_long_names_for_one_ds_set =\
                get_map_device_server_id_to_plugin_long_names()[one_ds_id]
            all_plugins_long_names_for_one_ds_list =\
                list(all_plugins_long_names_for_one_ds_set)
            all_plugins_long_names_for_one_ds_csv =\
                ",".join(all_plugins_long_names_for_one_ds_list)
            with open(one_host_one_ds_id_info_filename, 'a') as file:
                file.write("plugins=" + all_plugins_long_names_for_one_ds_csv +
                           "\n")
                file.write("karabo_user=" + one_karabo_user + "\n")
                file.write("karabo_fw=" + karabo_fw_version + "\n")

            # copy files from server to remote
            sftp_client.put("{}/{}".format(os.getcwd(),
                                           one_host_one_ds_id_info_filename),
                            "{}/{}".format(download_parent_dir,
                                           one_host_one_ds_id_info_filename))
            # small time sleep so scripts are copied fully
            time.sleep(2)

        #copy files from server to remote
        sftp_client.put("{}/{}".format(os.getcwd(), config_filename),
                        "{}/{}".format(download_parent_dir,
                                       config_filename))
        sftp_client.put("{}/{}".format(os.getcwd(), repos_filename),
                        "{}/{}".format(download_parent_dir,
                                       repos_filename))
        sftp_client.put("{}/{}".format(os.getcwd(), one_host_info_filename),
                        "{}/{}".format(download_parent_dir,
                                       one_host_info_filename))

        # small time sleep so scripts are copied fully before being executed
        time.sleep(2)
        client.close()

    #make installation scripts executable and execute pssh/parallelssh script
    os.system("chmod +x {} {}".format(downloadandinstall_script_filename,
                                      setupdeviceservers_script_filename))

    os.system("bash parassh.sh")

    return True


def exec_command_oneliner(conn, cmd):
    """

    :param conn:
    :param cmd:
    :return:
    """
    try:
        assert isinstance(cmd, str)
        stdin, stdout, stderr = conn.exec_command(cmd)
        if stdout.channel.recv_exit_status():
            print("error happened in executing cmd oneliner" + stderr.read())
        else:
            return stdout.readline().rstrip()
    except TypeError:
        print("typeerror in executing command. could be that"
              "file doesn't exist or path is not correct?"
              "cmd type: " + type(cmd))


def check_url(url):
    """
    Check if a URL exists without downloading the whole file.
    We only check the URL header.
    """
    good_codes = {http.client.OK, http.client.FOUND,
                  http.client.MOVED_PERMANENTLY}
    return get_server_status_code(url) in good_codes


def get_server_status_code(url):
    """
    Download just the header of a URL and
    return the server's status code.
    """
    host, path = urllib.request.urlparse(url)[1:3]  # elems [1] and [2]
    try:
        conn = httplib2.HTTPConnectionWithTimeout(host)
        conn.request('HEAD', path)
        return conn.getresponse().status
    except Exception:
        return None


################################################################################
####                           START                                        ####
################################################################################
def start_main(args):
    """

    :param args:
    :return:
    """
    return 0


################################################################################
####                           STOP                                         ####
################################################################################
def stop_main(args):
    """

    :param args:
    :return:
    """
    return 0


################################################################################
####                           UNINSTALL                                    ####
################################################################################
def uninstall_main(args):
    """

    :param args:
    :return:
    """
    return 0


################################################################################
####                           UPDATE                                       ####
################################################################################
def update_main(args):
    """

    :param args:
    :return:
    """
    return 0


################################################################################
####                           CHECK                                        ####
################################################################################
def check_main(args):
    """

    :param args:
    :return:
    """
    return 0


# supported schemas by this script
    # add new versions and remove obsolete versions from this array if required
__SUPPORTED_SCHEMA_VERSIONS__ = [1]

# dictionary with supported schemas
# key is the version number, value is the schema itself
#
# All schemas MUST HAVE defined property schema_version according to the
# definition below, where the N must be replaced
# by the actual version number of the schema
#
# "schema_version": {
# "description": "Indicates version of the schema",
# "enum": [N]
# }

__SCHEMA__ = {1: '''\
    {
        "$schema": "http://json-schema.org/draft-04/schema",
        "type":"object",
        "definitions":{
            "repository":{
                "allOf": [
                    {
                        "type": "string",
                        "format":"uri"
                    },
                    {
                        "type":"string",
                        "pattern":"(^file:/{3})|(^http:/{2})|(^https:/{2})|(^ftp:/{2})"
                    }
                ]
            },
            "user":{
            "type":"string",
                "pattern" : "^[A-Za-z0-9]+(?:[_-][A-Za-z0-9]+)*$",
                "maxLength": 8
            },
            "filesystem_path":{
                "type":"string",
                "pattern": "^(/[^/]+)+$"
            },
            "version":{
                "type":"string",
                "pattern": "^([0-9]|[0-9]{2})\\\.([0-9]|[0-9]{2})\\\.([0-9]|[0-9]{2}|[A-Za-z0-9_-]+)*$"
            }
        },

    "required": ["schema_version", "installation_name", "generation_date", "karabo_repositories", "karabo_packages_repositories", "karabo_dependencies_repositories", "broker","defaults","hosts"],
    "properties":{
            "schema_version": {
                "description": "Indicates version of the schema",
                "enum": [1]
            },
            "installation_name": {
                "description": "Must be unique name identifying karabo installation within the whole site (e.g. XFEL.EU). It is used as a broker topic name",
                "type":"string",
                "pattern":"^[A-Za-z0-9]+(?:[_-][A-Za-z0-9]+)*$"
            },
            "generation_date": {
                "description": "Date/Time when the file was generated",
                "type":"string",
                "pattern": "^(([0-9]{4})-(0[1-9]|1[0-2])-([12][0-9]|0[1-9]|3[01])T([01][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9])([\\\.,][0-9]+(?!:))?([zZ]|([+-])([01][0-9]|2[0-3]):([0-5][0-9]))?|([0-9]{4})(0[1-9]|1[0-2])([12][0-9]|0[1-9]|3[01])T([01][0-9]|2[0-3])([0-5][0-9])([0-5][0-9])([\\\.,][0-9]+(?!:))?([zZ]|([+-])([01][0-9]|2[0-3])([0-5][0-9]))?)$"
            },
            "karabo_repositories": {
                "description": "List of repositories with Karabo Framework",
                "type":"array",
                "minitems":1,
                "items": {
                    "description":"Karabo framework repository",
                    "$ref": "#/definitions/repository"
                }
            },
            "karabo_packages_repositories": {
                "description": "List of repositories with Karabo packages",
                "type": "array",
                "minitems": 1,
                "items": {
                    "description":"Karabo package repository",
                    "$ref": "#/definitions/repository"
                }
            },
            "karabo_dependencies_repositories": {
                "description": "List of repositories for dependencies of Karabo and/or Karabo packages",
                "type": "array",
                "minitems": 1,
                "items": {
                "description":"Karabo dependencies repository",
                "$ref": "#/definitions/repository"
                }
            },
            "broker": {
                "description":"Karabo broker. Note: Topic name is defined by installation_name.",
                "type":"object",
                "required": ["hostname","port"],
                "properties":{
                    "hostname": {
                        "description":"Hostname running broker instance",
                        "type":"string",
                        "format": "hostname"
                    },
                    "port": {
                        "description":"Broker port",
                        "type":"integer",
                        "minimum": 1024,
                        "maximum": 49151
                    }
                },
                "additionalProperties": false
            },
            "defaults": {
                "description": "Default values, may be overwritten",
                "type":"object",
                "required": ["install_prefix","karabo_user","karabo_version","login_user","run_prefix","autostart"],
                "properties":{
                    "login_user": {
                        "description": "Default username for login to individual hosts (i.e. ssh)",
                        "$ref": "#/definitions/user"
                    },
                    "install_prefix": {
                        "description": "Default prefix for the karabo installation",
                        "$ref": "#/definitions/filesystem_path"
                    },
                    "karabo_version": {
                        "description": "Default version of the karaboFramework",
                        "type":"string",
                        "$ref": "#/definitions/version"
                    },
                    "karabo_user": {
                        "description": "Default user who owns the run directories and device server processes",
                        "$ref": "#/definitions/user"
                    },
                    "run_prefix": {
                        "description": "Default prefix for run directories",
                        "$ref": "#/definitions/filesystem_path"
                    },
                    "autostart": {
                        "description": "Default flag defining if device server is started on host reboot",
                        "type":"boolean"
                    }
                },
                "additionalProperties": false
            },
            "hosts": {
                "description": "List of computers used by the installation",
                "type":"array",
                "items": {
                    "type":"object",
                    "required": ["hostname", "device_servers"],
                    "properties":{
                        "hostname": {
                            "description": "Host name",
                            "type":"string",
                            "format": "hostname"
                        },
                        "login_user": {
                            "description": "Username for login to this host (i.e. ssh)",
                            "$ref": "#/definitions/user"
                        },
                        "install_prefix": {
                            "description": "Prefix for the karabo installation on this host",
                            "$ref": "#/definitions/filesystem_path"
                        },
                        "karabo_user": {
                            "description": "User who owns the run directories and device server processes on this host",
                            "$ref": "#/definitions/user"
                        },
                        "device_servers": {
                            "description": "List of device servers on this host",
                            "type":"array",
                            "required": ["server_id", "plugins"],
                            "items": {
                                "type":"object",
                                "properties":{
                                    "server_id": {
                                        "description": "Device server name",
                                        "type":"string",
                                        "pattern" : "^[A-Za-z0-9]+(?:[_-][A-Za-z0-9]+)*$"
                                    },
                                    "autostart": {
                                        "description": "Flag defining if this device server is started on host reboot",
                                        "type":"boolean"
                                    },
                                    "karabo_user": {
                                        "description": "User who owns the run directory and device server process",
                                        "$ref": "#/definitions/user"
                                    },
                                    "configuration": {
                                        "description": "Configuration parameters for this device server.",
                                        "type":"string"
                                    },
                                    "run_prefix": {
                                        "description": "Prefix for run directory",
                                        "$ref": "#/definitions/filesystem_path"
                                    },
                                    "karabo_version": {
                                        "description": "Version of the Karabo framework for this device server",
                                        "$ref": "#/definitions/version"
                                    },
                                    "plugins": {
                                        "description": "List of plugins to be run on the devices server",
                                        "type":"array",
                                        "items": {
                                            "type":"object",
                                            "required": ["name","version"],
                                            "properties":{
                                                "name": {
                                                    "description": "Plugin name",
                                                    "type":"string",
                                                    "pattern" : "^[A-Za-z0-9]+(?:[_-][A-Za-z0-9]+)*$"
                                                },
                                                "version": {
                                                    "description": "Plugin version",
                                                    "$ref": "#/definitions/version"
                                                }
                                            },
                                            "additionalProperties": false
                                        }
                                    }
                                },
                                "additionalProperties": false
                            }
                        }
                    },
                    "additionalProperties": false
                }
            }
    },
        "additionalProperties": false
    }

'''
}


################################################################################
####                           MAIN                                         ####
################################################################################

if __name__ == "__main__":
    main()
