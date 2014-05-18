#!/usr/bin/env python

import sys
import argparse
import logging
import json
import pprint
from jsonschema import Draft4Validator
from jsonschema import FormatChecker
import os
import socket

__author__="wrona"
__date__ ="$May 11, 2014 5:32:08 PM$"

__TRACE__ = False

def main():
    

    parser = argparse.ArgumentParser(prog='karabo-server-control', description='Karabo server control. Tool to manage Karabo installation')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--verbose','-v',action='count', help='set verbosity level (-v, -vv)')
    group.add_argument('--quiet','-q',action='store_true', help='do not display anything')
    group.add_argument('--just-print','-n',action='store_true', help='simulate what the script would do on hosts')
    parser.add_argument('--host', '-H',action='store', help='run the tool as only this host would be defined in configuration file')

    
    subparsers = parser.add_subparsers()

    validate_parser = subparsers.add_parser('validate')
    validate_parser.add_argument('file', help='JSON file with configuration')
    validate_parser.set_defaults(func=validate)


    install_parser = subparsers.add_parser('install')
    install_parser.add_argument('file', help='JSON file with configuration')
    install_parser.set_defaults(func=install)
    
    update_parser = subparsers.add_parser('update')
    update_parser.add_argument('file_new', help='JSON file with new configuration')
    update_parser.add_argument('file_latest', help='JSON file with the latest applied configuration')
    update_parser.set_defaults(func=update)


    uninstall_parser = subparsers.add_parser('uninstall')
    uninstall_parser.add_argument('file_latest', help='JSON file with the latest applied configuration')
    uninstall_parser.set_defaults(func=uninstall)

    check_parser = subparsers.add_parser('check')
    check_parser.add_argument('file', help='JSON file with configuration')
    check_parser.set_defaults(func=check)

    start_parser = subparsers.add_parser('start')
    start_parser.set_defaults(func=start)

    stop_parser = subparsers.add_parser('stop')
    stop_parser.set_defaults(func=stop)    
    
    args = parser.parse_args()
    
    log_level = logging.WARN    
    if args.verbose == 1:
        log_level = logging.INFO
    if args.verbose == 2:
        log_level = logging.DEBUG
    if args.verbose == 3:
        global __TRACE__
        __TRACE__ = True
        log_level = logging.DEBUG
    

    if args.quiet:
        log_level=logging.CRITICAL
    
    if args.just_print:
        log_level=logging.INFO
        
    #logging.basicConfig(format='%(asctime)s %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p', level=log_level)
    logging.basicConfig(format='%(message)s', level=log_level)
                          
    trace('Command line arguments: ' + pprint.pformat(sys.argv) )
    trace('Parsed arguments: ' +  str(args) ) 
        
        
    ret = args.func(args)    
    exit(ret)
    

def validate(args):
    '''
    Validate if the configuration file has correct syntax and if it complies to the JSON schema
    '''
    
    logging.debug('sub-command: validate ' +  args.file)
    config = validate_configuration_file_against_schema(args.file)
    if config:
        logging.info("Configuration complies to the schema")
    else:        
        logging.warn("Configuration does not comply to the schema")
        return 1

    if not validate_configuration(config):
        logging.warn("Configuration is not valid")
        return 1
    
    logging.info("Configuration is valid")
    return 0
    
def install(args):    
    """
    Install fresh configuration defined in the file
    """    
    logging.debug('sub-command: install ' + args.file )
    
    config = validate_configuration_file_against_schema(args.file)
    if config:
        logging.info("Configuration complies to the schema")
    else:        
        logging.warn("Configuration does not comply to the schema")
        return 1
    
    if not validate_configuration(config):
        logging.warn("Configuration is not valid")        
        return 1
    
    

    
    
    
            
def update(args):
    """
    Update configuration. The function requires two configurations: latest already applied to hosts
    and new one. The function figures out what has changed and only applies these changes. 
    """
    
    
    logging.debug('sub-command: update ' + args.file_new + ' ' + args.file_latest)
        
    config_latest = validate_configuration_file_against_schema(args.file_latest)
    config_new = validate_configuration_file_against_schema(args.file_new)
    return 0

    
def uninstall(args):
    """
    Uninstall configuration. The configuration file must contain the latest applied configuration.
    """
    
    logging.debug( 'sub-command: uninstall ' + args.file_latest)
    return 0
    
    
def check(args):
    """
    Check if the configuration defined in the file is applied to the hosts
    """    
    logging.debug('sub-command: check ' + args.file)
    return 0
    
def start(args):
    """
    Start device server
    """
    logging.debug('sub-command: start ')
    return 0

def stop(args):
    """
    Stop device servers
    """    
    logging.debug('sub-command: stop ')
    return 0



def validate_configuration_file_against_schema(file):
    
    data = {}
    
    try: 
        data = load_json(file)    
    except ValueError, e:
        logging.warn("Syntax error in JSON file: " + file)
        logging.info(str(e))
        return {}
        
    
    
    logging.debug('====== Loaded configuration ======')
    logging.debug(pprint.pformat(data))

    if u'schema_version' in data.keys():
        schema_version = data['schema_version']
        if type(schema_version) != type(1) :
            logging.warn("Schema version must be an integer value")
            return {}
        if not schema_version in __SUPPORTED_SCHEMA_VERSIONS__:
            logging.warn("schema version: " + str(schema_version) +  " not supported by this tool")
            return {}
    else:
        logging.warn("schema version not defined in the JSON file. Check the configuration file.")
        return {}
        
    logging.debug("retrieving schema, version " + str(schema_version) )
        
    schema = {}
    try:
        schema = get_schema(schema_version)
    except ValueError, e:
        logging.warn("Syntax error in schema definition. Contact the author of the tool")
        logging.warn(str(e))
        return {}
            
    logging.debug("checking JSON schema...")
    Draft4Validator.check_schema(schema)
    
    trace("====== Loaded schema ======")
    trace(pprint.pformat(schema))
            
    logging.debug("validating JSON file...")
    v = Draft4Validator(schema,format_checker=FormatChecker())
 
    if v.is_valid(data):
        return data
    else:
        errors = v.iter_errors(data)
        for error in  sorted(errors, key=str):                
            logging.warn(pprint.pformat(error.message))
    
    return {}


def validate_configuration(config):

    logging.debug('checking if broker hostname can be resolved...')    
    if not hostname_resolves(config['broker']['hostname']):
        logging.info("broker hostname could not be resolved")
        return False
    else:
        logging.info("broker hostname could be resolved")
        
    logging.debug('checking if hostnames are unique...')    
    
    if not are_hostnames_unique(config):
        logging.warn("multiple definition for hosts")
        return False
    
    logging.debug('checking if all hostnames can be resolved...')    
    hosts = get_hostnames(config)
    if not hostnames_resolve(hosts):
        logging.warn("hostnames could not be resolved")
        return False
    else:
        logging.info("hostnames could be resolved")

    if not are_device_servers_unique(config):
        logging.warn('multiple definition of device_server')
        return False
    
    return True
    


def are_device_servers_unique(config):
    device_servers_ids = []
    for host in config['hosts']:
        for device_server in host['device_servers']:
            device_server_id = device_server['server_id']
            if device_server_id in device_servers_ids:
                logging.warn('device_server.server_id ' + device_server_id + ' defined multiple times')
                return False
            device_servers_ids.append(device_server_id)
    return True

def are_hostnames_unique(config):
    hostnames = []
    for host in config['hosts']:
        hostname = host['hostname']
        trace('are_hostnames_unique: host  ' + hostname)
        trace('are_hostnames_unique: hosts ' + pprint.pformat(hostnames))
        if hostname in hostnames:
            logging.warn('hostname ' + hostname + ' defined multiple times')
            return False
        hostnames.append(hostname)    
    return True
    

def get_hostnames(config):
    hosts = []
    for host in config['hosts']:
        hosts.append(host['hostname'])
    logging.debug("hostnames: " + str(hosts))
    return hosts

    
def hostname_resolves(hostname):
    logging.debug('checking if host ' + hostname + ' can be resolved')
    try:
        socket.gethostbyname(hostname)
        return True
    except socket.error:
        return False
    
def hostnames_resolve(hostnames):
    for hostname in hostnames:
        if not hostname_resolves(hostname):
            logging.warn('hostname ' + hostname + ' cannot be resolved')
            return False    
    return True
        
    
    
    
    
def load_json( filename ):
    json_file = open(filename)
    data = json.load(json_file)
    json_file.close()
    return data


def save_json( data, filename ):
    json_file = open(filename, "w")
    json.dump(data, json_file, indent=2)
    json_file.close()









def get_schema(version):
    
    if version in __SCHEMA__.keys():
        schema_string = __SCHEMA__[version ]
        return json.loads(schema_string)
    
    return None




def trace(message):
    global __TRACE__
    if __TRACE__ == True:
        logging.debug(message)

# supported schemas by this script
# add new versions and remove obsolete versions from this array if required
__SUPPORTED_SCHEMA_VERSIONS__ = [1]

# dictionary with supported schemas
# key is the version number, value is the schema itself
#
# All schemas MUST HAVE defined property schema_version according to the definition below, where the N must be replaced 
# by the actual version number of the schema
#
#            "schema_version": {
#                "description": "Indicates version of the schema",
#                "enum": [N]
#            }

__SCHEMA__ = { 1 : '''\
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

	"required": ["schema_version", "installation_name", "generation_date", "karabo_repositories", "karabo_packages_repositories", "broker","defaults","hosts"],
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
                "description":"List of repositories with Karabo packages",
                "type":"array",
                "minitems":1,
                "items": { 
                    "description":"Karabo package repository",
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


if __name__ == "__main__":
    main()
