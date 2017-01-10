#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from xml.etree.ElementTree import Element, SubElement, parse, tostring

from traits.api import Enum, Instance, Int, List, String, Tuple

from karabo.common.savable import BaseSavableModel
from .bases import BaseProjectObjectModel
from .device import DeviceConfigurationModel


class DeviceInstanceModel(BaseSavableModel):
    """ A device which can be instantiated by a server
    """
    # The class ID of the device
    class_id = String
    # The device ID of the instantiated device
    instance_id = String
    # If the device is already online, should it be ignored or restarted?
    if_exists = Enum('ignore', 'restart')
    # A list of references to possible configurations
    configs = List(Instance(DeviceConfigurationModel))
    # UUID/Rev of the currently active configuration
    active_config_ref = Tuple(String, Int)
    # The current status of the device
    status = String('offline', transient=True)

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

    def _class_id_changed(self):
        """When ``class_id`` changes, make sure the configurations are
        compatible.
        """
        configs = [cnf for cnf in self.configs
                   if cnf.class_id == self.class_id]
        self.configs = configs

    def _configs_changed(self, name, old, new):
        """ Traits notification handler for list assignment
        """
        self._update_config_listeners(old, new)

    def _configs_items_changed(self, event):
        """ Traits notification handler for list inserts/removes
        """
        self._update_config_listeners(event.removed, event.added)

    def _update_config_listeners(self, removed, added):
        """ Whenever the List of ``configs`` is changed, the listeners need to
        be updated

        :param removed: A list of removed ``DeviceConfigurationModel`` objects
        :param added: A list of added ``DeviceConfigurationModel`` objects
        """
        active_ref = self.active_config_ref
        for config_model in removed:
            # Handle our active configuration leaving!
            if (config_model.uuid, config_model.revision) == active_ref:
                self.active_config_ref = ('', 0)
            # Remove listener for ``revision`` change event
            config_model.on_trait_change(self._update_active_revision,
                                         'revision', remove=True)

        rejects = []
        for config_model in added:
            # Reject incompatible configurations
            if self.class_id and config_model.class_id != self.class_id:
                rejects.append(config_model)
            # Add listener for ``revision`` change event
            config_model.on_trait_change(self._update_active_revision,
                                         'revision')

        if rejects:
            # Remove the incompatible configurations!
            # This is recursive, but in each call `added` will be empty
            for config_model in rejects:
                self.configs.remove(config_model)

            # Wait until we've made ourself consistent before raising
            raise ValueError("Incompatible configuration(s) added to device!")

    def _update_active_revision(self, obj, name, old, new):
        """ Whenever the revision of a ``DeviceConfigurationModel`` is changed
        the ``active_config_ref`` might refer to it and needs to be updated

        :param obj: The ``DeviceInstanceModel`` object which revision was
                    changed
        :param name: The name of the trait
        :param old: The old value of the trait
        :param new: The new value of the trait
        """
        uuid, revision = self.active_config_ref
        if (obj.uuid == uuid) and (revision == old):
            self.active_config_ref = uuid, new


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
    def _read_configs(element):
        traits = {
            'uuid': element.get('uuid'),
            'revision': int(element.get('revision')),
            'class_id': element.get('class_id', ''),
            'initialized': False,
        }
        return DeviceConfigurationModel(**traits)

    def _read_device_instance(element):
        traits = {
            'class_id': element.get('class_id'),
            'instance_id': element.get('instance_id'),
            'if_exists': element.get('if_exists', 'ignore'),
            'active_config_ref': (element.get('active_uuid'),
                                  int(element.get('active_rev'))),
            'initialized': True
        }
        configs = [_read_configs(e) for e in element.findall('config')]
        return DeviceInstanceModel(configs=configs, **traits)

    document = parse(io_obj)
    root = document.getroot()
    devices = [_read_device_instance(e) for e in root.findall('device')]
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
    def _write_configs(obj, parent):
        element = SubElement(parent, 'config')
        element.set('class_id', obj.class_id)
        element.set('uuid', obj.uuid)
        element.set('revision', str(obj.revision))

    def _write_device_instance(obj, parent):
        element = SubElement(parent, 'device')
        active_uuid, active_rev = obj.active_config_ref
        element.set('class_id', obj.class_id)
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

    return tostring(root, encoding='unicode')
