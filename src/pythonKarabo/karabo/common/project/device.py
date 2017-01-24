#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from xml.etree.ElementTree import Element, parse, SubElement, tostring

from traits.api import Enum, Instance, Int, List, String, Tuple

from .bases import BaseProjectObjectModel
from .const import (
    PROJECT_DB_TYPE_DEVICE_CONFIG, PROJECT_DB_TYPE_DEVICE_INSTANCE
)
from .device_config import DeviceConfigurationModel


class DeviceInstanceModel(BaseProjectObjectModel):
    """ A device instance of a device class with an active configuration on a
    server
    """
    # The class ID of the device
    class_id = String
    # The device ID of the instantiated device
    instance_id = String
    # The server ID. Transient, because a parent DeviceServerModel will fill it
    server_id = String(transient=True)

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


def read_device(io_obj):
    """ A reader for device models
    """
    def _read_device_config(element):
        traits = {
            'class_id': element.get('class_id'),
            'uuid': element.get('uuid'),
            'revision': int(element.get('revision')),
        }
        return DeviceConfigurationModel(**traits)

    document = parse(io_obj)
    root = document.getroot()
    configs = [_read_device_config(e)
               for e in root.findall(PROJECT_DB_TYPE_DEVICE_CONFIG)]
    traits = {
        'class_id': root.get('class_id'),
        'instance_id': root.get('instance_id'),
        'if_exists': root.get('if_exists'),
        'configs': configs,
        'active_config_ref': (root.get('active_uuid'),
                              int(root.get('active_rev'))),
    }
    model = DeviceInstanceModel(**traits)
    model.initialized = True  # Do this last to avoid triggering `modified`
    return model


def write_device(model):
    """ A writer for device models
    """
    def _write_config(obj, parent):
        element = SubElement(parent, PROJECT_DB_TYPE_DEVICE_CONFIG)
        element.set('class_id', obj.class_id)
        element.set('uuid', obj.uuid)
        element.set('revision', str(obj.revision))

    root = Element(PROJECT_DB_TYPE_DEVICE_INSTANCE)
    active_uuid, active_rev = model.active_config_ref
    root.set('class_id', model.class_id)
    root.set('instance_id', model.instance_id)
    root.set('if_exists', model.if_exists)
    root.set('active_uuid', active_uuid)
    root.set('active_rev', str(active_rev))
    for config in model.configs:
        _write_config(config, root)

    return tostring(root, encoding='unicode')
