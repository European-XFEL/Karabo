#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from xml.etree.ElementTree import Element, SubElement, parse, tostring

from traits.api import (HasStrictTraits, Enum, Instance, Int, List, String,
                        Tuple)

from .bases import BaseProjectObjectModel
from .device import DeviceConfigurationModel


class DeviceInstanceModel(HasStrictTraits):
    """ A device which can be instantiated by a server
    """
    # The device ID of the instantiated device
    instance_id = String
    # If the device is already online, should it be ignored or restarted?
    if_exists = Enum('ignore', 'restart')
    # A list of references to possible configurations
    configs = List(Instance(DeviceConfigurationModel))
    # UUID/Rev of the currently active configuration
    active_config_ref = Tuple(String, Int)

    def select_config(self, uuid, revision):
        """ Find the `DeviceConfigurationModel` matching the given `uuid` and
        `revision`.

        :param uuid: A UUID as a String
        :param revision: A revision number as Int
        :return: The `DeviceConfigurationModel` object, if found, else `None`
        """
        for dev_config in self.configs:
            if (dev_config.uuid, dev_config.revision) == (uuid, revision):
                return dev_config


class DeviceServerModel(BaseProjectObjectModel):
    """ An object representing a device server
    """
    # The device ID of the instantiated server
    server_id = String
    # The host on which the server runs
    host = String
    # A list of possible devices for the server
    devices = List(Instance(DeviceInstanceModel))

    def _server_id_changed(self, old, new):
        self.simple_name = new


def read_device_server(io_obj):
    """ A reader for device server models
    """
    def _read_configs(element):
        traits = {
            'uuid': element.get('uuid'),
            'revision': int(element.get('revision')),
            'initialized': False,
        }
        return DeviceConfigurationModel(**traits)

    def _read_device_instance(element):
        traits = {
            'instance_id': element.get('instance_id'),
            'if_exists': element.get('if_exists', 'ignore'),
            'active_config_ref': (element.get('active_uuid'),
                                  int(element.get('active_rev'))),
        }
        configs = [_read_configs(e) for e in element.findall('config')]
        return DeviceInstanceModel(configs=configs, **traits)

    document = parse(io_obj)
    root = document.getroot()
    devices = [_read_device_instance(e) for e in root.findall('device')]
    traits = {
        'server_id': root.get('server_id'),
        'host': root.get('host'),
        'devices': devices
    }
    return DeviceServerModel(**traits)


def write_device_server(model):
    """ A writer for device server models
    """
    def _write_configs(obj, parent):
        element = SubElement(parent, 'config')
        element.set('uuid', obj.uuid)
        element.set('revision', str(obj.revision))

    def _write_device_instance(obj, parent):
        element = SubElement(parent, 'device')
        active_uuid, active_rev = obj.active_config_ref
        element.set('instance_id', obj.instance_id)
        element.set('if_exists', obj.if_exists)
        element.set('active_uuid', active_uuid)
        element.set('active_rev', str(active_rev))
        for config in obj.configs:
            _write_configs(config, element)

    root = Element('device_server')
    root.set('server_id', model.server_id)
    root.set('host', model.host)
    for device in model.devices:
        _write_device_instance(device, root)

    return tostring(root, encoding='utf-8')
