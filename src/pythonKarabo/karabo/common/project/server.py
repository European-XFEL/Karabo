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


class DeviceServerModel(BaseProjectObjectModel):
    """ An object representing a device server
    """
    # The device ID of the instantiated server
    server_id = String
    # A list of possible devices for the server
    devices = List(Instance(DeviceInstanceModel))


def read_device_server(io_obj):
    """ A reader for device server models
    """
    def _read_config_ref(element):
        uuid = element.get('uuid')
        revision = int(element.get('revision'))
        return DeviceConfigurationModel(uuid=uuid, revision=revision)

    def _read_device_instance(element):
        instance_id = element.get('instance_id')
        if_exists = element.get('if_exists', 'ignore')
        configs = [_read_config_ref(e) for e in element.findall('config')]
        return DeviceInstanceModel(instance_id=instance_id,
                                   if_exists=if_exists,
                                   configs=configs)

    document = parse(io_obj)
    root = document.getroot()
    server_id = root.get('server_id')
    devices = [_read_device_instance(e) for e in root.findall('device')]
    return DeviceServerModel(server_id=server_id, devices=devices)


def write_device_server(model):
    """ A writer for device server models
    """
    def _write_config_ref(obj, parent):
        element = SubElement(parent, 'config')
        element.set('uuid', obj.uuid)
        element.set('revision', str(obj.revision))

    def _write_device_instance(obj, parent):
        element = SubElement(parent, 'device')
        element.set('instance_id', obj.instance_id)
        element.set('if_exists', obj.if_exists)
        for config in obj.configs:
            _write_config_ref(config, element)

    root = Element('device_server')
    root.set('server_id', model.server_id)
    for device in model.devices:
        _write_device_instance(device, root)

    return tostring(root)
