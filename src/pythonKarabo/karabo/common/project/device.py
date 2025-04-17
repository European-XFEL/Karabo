#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from xml.etree.ElementTree import Element, SubElement, parse, tostring

from traits.api import Instance, List, String, on_trait_change

from .bases import BaseProjectObjectModel
from .const import (
    PROJECT_DB_TYPE_DEVICE_CONFIG, PROJECT_DB_TYPE_DEVICE_INSTANCE)
from .device_config import DeviceConfigurationModel


class DeviceInstanceModel(BaseProjectObjectModel):
    """A device instance of a device class with an active configuration on a
    server
    """

    # The class ID of the device
    class_id = String
    # The device ID of the instantiated device
    instance_id = String
    # The server ID. Transient, because a parent DeviceServerModel will fill it
    server_id = String(transient=True)

    # A list of references to possible configurations
    configs = List(Instance(DeviceConfigurationModel))
    # UUID of the currently active configuration
    active_config_ref = String

    def select_config(self, uuid):
        """Find the `DeviceConfigurationModel` matching the given `uuid`

        :param uuid: A UUID as a String
        :return: The `DeviceConfigurationModel` object, if found, else `None`
        """
        for dev_config in self.configs:
            if dev_config.uuid == uuid:
                return dev_config

    def _class_id_changed(self):
        """When ``class_id`` changes, make sure the configurations are
        compatible.
        """
        for config_model in self.configs:
            config_model.class_id = self.class_id

    def _configs_changed(self, name, old, new):
        """Traits notification handler for list assignment"""
        self._update_config_listeners(old, new)

    def _configs_items_changed(self, event):
        """Traits notification handler for list inserts/removes"""
        self._update_config_listeners(event.removed, event.added)

    def _update_config_listeners(self, removed, added):
        """Whenever the List of ``configs`` is changed, the listeners need to
        be updated

        :param removed: A list of removed ``DeviceConfigurationModel`` objects
        :param added: A list of added ``DeviceConfigurationModel`` objects
        """
        active_ref = self.active_config_ref
        for config_model in removed:
            # Handle our active configuration leaving!
            if config_model.uuid == active_ref:
                self.active_config_ref = ""
            # Remove listeners for ``uuid`` change event
            config_model.on_trait_change(
                self._update_active_uuid, "uuid", remove=True
            )

        for config_model in added:
            if self.class_id:
                config_model.class_id = self.class_id
            # Add listeners for ``uuid`` change event
            config_model.on_trait_change(self._update_active_uuid, "uuid")

    def _update_active_uuid(self, obj, name, old, new):
        """Whenever the UUID of a ``DeviceConfigurationModel`` is changed
        the ``active_config_ref`` might refer to it and needs to be updated

        :param obj: The ``DeviceInstanceModel`` object on which ``uuid`` was
                    changed
        :param name: The name of the trait
        :param old: The old value of the trait
        :param new: The new value of the trait
        """
        if self.active_config_ref == old:
            self.active_config_ref = new

    @on_trait_change("configs:initialized")
    def _update_configs_initialized(self):
        """Make sure to set ``initialized`` flag only to ``True`` whenever all
        ``DeviceConfigurationModel`` items are initialized
        """
        for conf in self.configs:
            if not conf.initialized:
                break
        else:
            self.initialized = True


def read_device(io_obj):
    """A reader for device models"""

    def _read_device_config(element):
        traits = {
            "class_id": element.get("class_id", ""),
            "uuid": element.get("uuid"),
            "initialized": False,
        }
        return DeviceConfigurationModel(**traits)

    document = parse(io_obj)
    root = document.getroot()
    configs = [
        _read_device_config(e)
        for e in root.findall(PROJECT_DB_TYPE_DEVICE_CONFIG)
    ]
    traits = {"configs": configs}
    # Read traits in a way which is resilient against missing data
    trait_names = {
        "class_id": "class_id",
        "instance_id": "instance_id",
        "active_config_ref": "active_uuid",
    }
    traits.update(
        {
            key: root.get(xmlkey)
            for key, xmlkey in trait_names.items()
            if root.get(xmlkey) is not None
        }
    )
    return DeviceInstanceModel(**traits)


def write_device(model):
    """A writer for device models"""

    def _write_config(obj, parent):
        element = SubElement(parent, PROJECT_DB_TYPE_DEVICE_CONFIG)
        element.set("class_id", obj.class_id)
        element.set("uuid", obj.uuid)
        # XXX: Protect old code. Remove this when domains are implemented.
        element.set("revision", "0")

    root = Element(PROJECT_DB_TYPE_DEVICE_INSTANCE)
    root.set("class_id", model.class_id)
    root.set("instance_id", model.instance_id)
    root.set("active_uuid", model.active_config_ref)
    # XXX: Protect old code. Remove this when domains are implemented.
    root.set("active_rev", "0")
    for config in model.configs:
        _write_config(config, root)

    return tostring(root, encoding="unicode")
