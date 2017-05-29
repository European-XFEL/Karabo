#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from traits.api import (HasStrictTraits, Bool, Event, Instance, Property,
                        String, WeakRef)

from karabo.middlelayer import Hash
from karabo_gui.configuration import Configuration
from karabo_gui.singletons.api import get_topology
from .tree import SystemTreeNode
from .util import clear_configuration_instance


class ProjectDeviceInstance(HasStrictTraits):
    """An abstraction of devices needed for projects and workflow design.
    No matter the online/offline state of a device, you can get a
    ``Configuration`` object which is valid and associated with the same
    device id.
    """
    # The current device id. Can be monitored for changes.
    device_id = String
    class_id = String
    server_id = String
    # Binary online/offline state of the device
    online = Bool
    # Current status of the online device (never the offline device)
    status = String('offline')
    # The current configuration for this device
    current_configuration = Property(Instance(Configuration))

    # A weak reference to the system topology device node
    device_node = WeakRef(SystemTreeNode, allow_none=True)

    # An event which is triggered whenever the configuration is updated
    configuration_updated = Event
    # An event which is triggered whenever the class schema changes
    schema_updated = Event

    # The online/offline configurations
    _initial_config_hash = Instance(Hash)
    _class_config = Instance(Configuration)
    _offline_dev_config = Instance(Configuration)
    _online_dev_config = Instance(Configuration)

    def __init__(self, device_id, class_id, server_id, init_hash):
        super(ProjectDeviceInstance, self).__init__()

        self._initial_config_hash = init_hash
        self._init_object_state(device_id, class_id, server_id)

    def destroy(self):
        """Disconnect slots which are connected to this object's methods
        """
        new_descriptor = self._descriptor_change_slot
        self._class_config.signalNewDescriptor.disconnect(new_descriptor)
        new_descriptor = self._online_descriptor_change_slot
        self._online_dev_config.signalNewDescriptor.disconnect(new_descriptor)
        status_changed = self._status_change_slot
        self._online_dev_config.signalStatusChanged.disconnect(status_changed)
        config_changed = self._config_change_slot
        self._online_dev_config.signalBoxChanged.disconnect(config_changed)
        self._offline_dev_config.signalBoxChanged.disconnect(config_changed)

        clear_configuration_instance(self._offline_dev_config)

    def rename(self, device_id='', class_id='', server_id=''):
        """Assign a new device_id, class_id, or server_id.
        """
        device_id = device_id or self.device_id
        class_id = class_id or self.class_id
        server_id = server_id or self.server_id

        # First check to see if anything is changing!
        if (device_id == self.device_id and class_id == self.class_id
                and server_id == self.server_id):
            return

        self.destroy()
        self._init_object_state(device_id, class_id, server_id)

    def set_project_config_hash(self, config_hash):
        """Forcibly set the offline configuration Hash of the device.
        """
        if self._offline_dev_config.descriptor is None or config_hash is None:
            return

        self._offline_dev_config.setDefault()
        self._offline_dev_config.fromHash(config_hash)

    # ---------------------------------------------------------------------
    # Traits Handlers

    def _get_current_configuration(self):
        """Traits Property getter for the current configuration
        """
        if (self.online and self._online_dev_config.serverId == self.server_id
                and self._online_dev_config.classId == self.class_id):
            return self._online_dev_config

        return self._offline_dev_config

    # ---------------------------------------------------------------------
    # Qt Slots

    def _config_change_slot(self):
        """The (possibly) current ``Configuration`` object has been edited by
        a user.
        """
        # Let the world know
        self.configuration_updated = True

    def _descriptor_change_slot(self, config):
        """The global class has received a new schema which needs to be set
        for the dependent device ``Configuration`` instances.
        """
        if self._offline_dev_config.descriptor is not None:
            # Save current offline configuration hash
            self._initial_config_hash, _ = self._offline_dev_config.toHash()
            self._offline_dev_config.redummy()
        self._offline_dev_config.descriptor = config.descriptor

        # Set values for offline configuration
        self._offline_dev_config.setDefault()
        if self._initial_config_hash is not None:
            self._offline_dev_config.fromHash(self._initial_config_hash)

        # Let the world know
        self.schema_updated = True

    def _online_descriptor_change_slot(self, config):
        """The online device has received a new schema.
        """
        # Let the world know
        self.schema_updated = True

    def _status_change_slot(self, box, status, error_flag):
        """The `_online_dev_config` trait has changed its status. Check if it's
        online
        """
        self.online = self._online_dev_config.isOnline()
        if self.online:
            self.status = self._update_online_status(box, status, error_flag)
        else:
            self.status = self._update_offline_status()

        # Keep the device_node pointer up-to-date
        self.device_node = self._online_dev_config.topology_node

    # ---------------------------------------------------------------------
    # utils

    def _init_object_state(self, device_id, class_id, server_id):
        """Initialize the object for a given device_id/server_id/class_id.
        """
        topology = get_topology()
        class_config = topology.get_class(server_id, class_id)
        online_device = topology.get_device(device_id)
        offline_device = Configuration(device_id, 'projectClass')
        offline_device.serverId = server_id
        offline_device.classId = class_id

        self._class_config = class_config
        self._online_dev_config = online_device
        self._offline_dev_config = offline_device

        # Connect to signals
        class_config.signalNewDescriptor.connect(self._descriptor_change_slot)
        online_device.signalNewDescriptor.connect(
            self._online_descriptor_change_slot)
        online_device.signalStatusChanged.connect(self._status_change_slot)
        online_device.signalBoxChanged.connect(self._config_change_slot)
        offline_device.signalBoxChanged.connect(self._config_change_slot)

        if class_config.descriptor is not None:
            self._descriptor_change_slot(class_config)

        # Remember the IDs (also notifies the outside world of changes)
        self.device_id = device_id
        self.class_id = class_id
        self.server_id = server_id

        # Update the online flag
        self.online = online_device.isOnline()
        # Update the status
        online_device.updateStatus()

    def _update_online_status(self, box, status, error_flag):
        """Return correct online status for given ``server_id`` and
        ``class_id``
        """
        conf = box.configuration
        if conf.serverId == self.server_id and conf.classId == self.class_id:
            return 'error' if error_flag else status
        return 'incompatible'

    def _update_offline_status(self):
        """Return correct offline status for given ``server_id`` and
        ``class_id``
        """
        topology = get_topology()
        server_key = 'server.{}'.format(self.server_id)
        attributes = topology.get_attributes(server_key)
        if attributes is None:
            return 'noserver'
        elif self.class_id not in attributes.get('deviceClasses', []):
            return 'noplugin'
        return 'offline'
