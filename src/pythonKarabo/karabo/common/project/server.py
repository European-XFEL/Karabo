#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from xml.etree.ElementTree import Element, parse, SubElement, tostring

from traits.api import Instance, List, String

from .bases import BaseProjectObjectModel
from .const import (
    PROJECT_DB_TYPE_DEVICE_INSTANCE, PROJECT_DB_TYPE_DEVICE_SERVER
)
from .device import DeviceInstanceModel


class DeviceServerModel(BaseProjectObjectModel):
    """ An object representing a device server
    """
    # The device ID of the instantiated server
    server_id = String
    # The host on which the server runs
    host = String
    # A list of possible devices for the server
    devices = List(Instance(DeviceInstanceModel))
    # The current status of the server
    status = String('offline', transient=True)

    def get_device_instance(self, instance_id):
        for dev in self.devices:
            if dev.instance_id == instance_id:
                return dev
        return None

    def _server_id_changed(self, old, new):
        self.simple_name = new


def read_device_server(io_obj):
    """ A reader for device server models
    """
    def _read_device(element):
        traits = {
            'uuid': element.get('uuid'),
            'revision': int(element.get('revision')),
            'initialized': False
        }
        return DeviceInstanceModel(**traits)

    document = parse(io_obj)
    root = document.getroot()
    devices = [_read_device(e)
               for e in root.findall(PROJECT_DB_TYPE_DEVICE_INSTANCE)]
    traits = {
        'server_id': root.get('server_id'),
        'host': root.get('host'),
        'devices': devices,
        'initialized': True
    }
    return DeviceServerModel(**traits)


def write_device_server(model):
    """ A writer for device server models
    """
    def _write_device(obj, parent):
        element = SubElement(parent, PROJECT_DB_TYPE_DEVICE_INSTANCE)
        element.set('uuid', obj.uuid)
        element.set('revision', str(obj.revision))

    root = Element(PROJECT_DB_TYPE_DEVICE_SERVER)
    root.set('server_id', model.server_id)
    root.set('host', model.host)
    for device in model.devices:
        _write_device(device, root)

    return tostring(root, encoding='unicode')
