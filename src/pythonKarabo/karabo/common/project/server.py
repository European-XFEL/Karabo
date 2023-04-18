#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from xml.etree.ElementTree import Element, SubElement, parse, tostring

from traits.api import Instance, List, String

from .bases import BaseProjectObjectModel
from .const import (
    PROJECT_DB_TYPE_DEVICE_INSTANCE, PROJECT_DB_TYPE_DEVICE_SERVER)
from .device import DeviceInstanceModel


class DeviceServerModel(BaseProjectObjectModel):
    """An object representing a device server"""

    # The device ID of the instantiated server
    server_id = String
    # The host on which the server runs
    host = String
    # A list of possible devices for the server
    devices = List(Instance(DeviceInstanceModel))
    # The current `ProxyStatus` object of the server
    status = Instance(object, transient=True)

    def get_device_instance(self, instance_id):
        for dev in self.devices:
            if dev.instance_id == instance_id:
                return dev
        return None

    def _server_id_changed(self, new):
        self.simple_name = new

        # Update the child DeviceInstanceModels!
        for dev in self.devices:
            dev.server_id = new

    def _update_device_server_ids(self, added):
        """Manage the `server_id` trait of device instances as they arrive."""
        for dev in added:
            dev.server_id = self.server_id

    def _devices_changed(self, new):
        self._update_device_server_ids(new)

    def _devices_items_changed(self, event):
        self._update_device_server_ids(event.added)


def read_device_server(io_obj):
    """A reader for device server models"""

    def _read_device(element, server_id):
        traits = {
            "uuid": element.get("uuid"),
            "server_id": server_id,  # Actually transient!
            "initialized": False,
        }
        return DeviceInstanceModel(**traits)

    document = parse(io_obj)
    root = document.getroot()
    server_id = root.get("server_id", "")
    devices = [
        _read_device(e, server_id)
        for e in root.findall(PROJECT_DB_TYPE_DEVICE_INSTANCE)
    ]
    traits = {
        "server_id": server_id,
        "host": root.get("host", ""),
        "devices": devices,
    }
    model = DeviceServerModel(**traits)
    model.initialized = True  # Do this last to avoid triggering `modified`
    return model


def write_device_server(model):
    """A writer for device server models"""

    def _write_device(obj, parent):
        element = SubElement(parent, PROJECT_DB_TYPE_DEVICE_INSTANCE)
        element.set("uuid", obj.uuid)
        # XXX: Protect old code. Remove this when domains are implemented.
        element.set("revision", "0")

    root = Element(PROJECT_DB_TYPE_DEVICE_SERVER)
    root.set("server_id", model.server_id)
    root.set("host", model.host)
    for device in model.devices:
        _write_device(device, root)

    return tostring(root, encoding="unicode")
