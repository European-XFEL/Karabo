#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 10, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from traits.api import (HasStrictTraits, Bool, Dict, Instance, Property,
                        on_trait_change)

from karabo.common.api import DeviceStatus
from karabo.middlelayer import Hash, Schema
from karabo_gui.alarms.api import ADD_ALARM_TYPES, REMOVE_ALARM_TYPES
from karabo_gui.configuration import BulkNotifications, Configuration
from karabo_gui.singletons.api import get_network
from .project_device import ProjectDeviceInstance
from .tree import SystemTree
from .util import clear_configuration_instance


class _DeviceNodeFinder(object):
    """A system tree visitor which finds the node for a specific device id.
    """
    def __init__(self, device_id):
        self.node = None
        self._device_id = device_id

    def __call__(self, node):
        if node.node_id == self._device_id:
            self.node = node


class SystemTopology(HasStrictTraits):
    """The Karabo System Topology

    This is a data structure containing all of the online devices and device
    servers in a running Karabo system. It is kept continually up-to-date by
    the connection to the GUI server.
    """

    # A high-level version of `_system_hash`. Used by GUI views.
    system_tree = Instance(SystemTree)

    # True if the system is online, False otherwise
    online = Property(Bool)

    # Mapping of (server id, class id) -> Schema
    _class_schemas = Dict

    # Mapping of (server id, class id) -> Configuration
    _class_configurations = Dict

    # Mapping of device_id -> Configuration
    _online_devices = Dict

    # Mapping of device_id -> ProjectDeviceInstance
    _project_devices = Dict

    # A Hash instance holding the entire current topology
    _system_hash = Instance(Hash, allow_none=True)

    # Required by the singleton system (see ``karabo_gui.singleton.api``)
    def __init__(self, parent=None):
        super(SystemTopology, self).__init__()

    def clear(self):
        """Clear all saved devices and classes
        """
        self.clear_project_devices()
        self.system_tree.clear_all()

        self._system_hash = None
        self._class_schemas = {}
        self._class_configurations = {}
        self._online_devices = {}

    def clear_project_devices(self):
        for dev in self._project_devices.values():
            dev.destroy()

        self._project_devices = {}

    def get_attributes(self, topology_path):
        """Return the attributes of a given node in the `_system_hash`.
        """
        if self._system_hash is None:
            return None

        try:
            return self._system_hash[topology_path, ...]
        except KeyError:
            return None

    def get_class(self, server_id, class_id):
        """Return the ``Configuration`` for a given class on a given server
        """
        key = (server_id, class_id)
        klass = self._class_configurations.get(key)
        if klass is None:
            path = "{}.{}".format(*key)
            klass = Configuration(path, 'class')
            klass.serverId = server_id
            klass.classId = class_id
            self._class_configurations[key] = klass

        statuses = (DeviceStatus.REQUESTED, DeviceStatus.SCHEMA)
        if klass.descriptor is None or klass.status not in statuses:
            get_network().onGetClassSchema(*key)
            klass.status = DeviceStatus.REQUESTED

        return klass

    def get_device(self, device_id):
        """Return the online configuration for a given device
        """
        device = self._online_devices.get(device_id)
        if device is None:
            device = Configuration(device_id, 'device')
            self._online_devices[device_id] = device
            device.updateStatus()

            # Get the system topology node, if it's there
            finder = _DeviceNodeFinder(device_id)
            self.visit_system_tree(finder)
            if finder.node:
                device.topology_node = finder.node

        statuses = (DeviceStatus.OFFLINE, DeviceStatus.REQUESTED)
        if device.descriptor is None and device.status not in statuses:
            get_network().onGetDeviceSchema(device_id)
            device.status = DeviceStatus.REQUESTED

        return device

    def get_project_device(self, device_id, class_id='', server_id='',
                           init_config=None):
        """Return a ``ProjectDeviceInstance`` for a device on a specific
        server.
        """
        if device_id not in self._project_devices:
            if class_id == '' or server_id == '':
                msg = ('Project device with id "{}" was requested without '
                       'specifying the class_id or server_id! Perhaps a '
                       'device exists in a scene workflow but not in an open '
                       'project?')
                raise RuntimeError(msg.format(device_id))

            instance = ProjectDeviceInstance(device_id, class_id, server_id,
                                             init_config)
            self._project_devices[device_id] = instance
        else:
            instance = self._project_devices[device_id]
            instance.rename(
                device_id=device_id, class_id=class_id, server_id=server_id
            )

        return self._project_devices[device_id]

    def get_schema(self, server_id, class_id):
        """Return the ``Schema`` for a given class on a given server
        """
        key = (server_id, class_id)
        return self._class_schemas.get(key)

    def visit_system_tree(self, visitor):
        """Walk every node in the system tree and run a `visitor` function on
        each item.
        """
        self.system_tree.visit(visitor)

    # ---------------------------------------------------------------------
    # Traits Handlers

    def _get_online(self):
        """The topology is considered to be online if `_system_hash` exists.
        """
        return self._system_hash is not None

    def _system_tree_default(self):
        """We must provide a default initializer since the singleton system
        will instantiate the once instance of this object.
        """
        return SystemTree()

    @on_trait_change('_project_devices:device_id')
    def _project_device_instance_id_changed(self, obj, name, old, new):
        """React to the device id of a project device changing.
        """
        msg = ('ProjectDeviceInstance changed its instance_id to a name which '
               'is already used by another device!')
        assert new not in self._project_devices, msg

        proj_device = self._project_devices.pop(old, None)
        if proj_device is not None:
            self._project_devices[new] = proj_device

    # ---------------------------------------------------------------------
    # Handlers for GUI Server Messages

    def class_schema_updated(self, server_id, class_id, schema):
        """Called when a `classSchema` message is received from the server.

        Returns the ``Configuration`` object associated with `schema`, if one
        exists and is initialized.
        """
        key = (server_id, class_id)
        if key not in self._class_configurations:
            print('Unrequested schema for classId {} arrived'.format(class_id))
            return None

        # Save a clean copy
        schema_copy = Schema()
        schema_copy.copy(schema)
        self._class_schemas[key] = schema_copy

        conf = self._class_configurations[key]
        if conf.descriptor is not None:
            # If the class schema has already arrived in the past, then we
            # should take no further action. We don't expect the schema of a
            # class on a running server to change. Device _instances_, however,
            # will change, but anyhow don't arrive in this handler.
            return None

        if len(schema.hash) > 0:
            # Set schema only, if data is available
            conf.setSchema(schema)
            # Set default values for configuration
            conf.setDefault()

        return conf

    def device_config_updated(self, device_id, config):
        """Called when a `deviceConfiguration` message is received from the
        server

        Returns the ``Configuration`` object associated with `device_id`, if
        one exists and is initialized.
        """
        device = self._online_devices.get(device_id, None)
        if device is None or device.descriptor is None:
            return None

        with BulkNotifications(device):
            device.fromHash(config)

        if device.status is DeviceStatus.SCHEMA:
            device.status = DeviceStatus.ALIVE
        if device.status is DeviceStatus.ALIVE and device.visible > 0:
            device.status = DeviceStatus.MONITORING

        return device

    def device_schema_updated(self, device_id, schema):
        """Called when a `deviceSchema` message is received from the server

        Returns the ``Configuration`` object associated with `schema`, if one
        exists and is initialized.
        """
        if device_id not in self._online_devices:
            print('Unrequested schema for device {} arrived'.format(device_id))
            return None

        conf = self._online_devices[device_id]
        # Schema already existent -> schema injected
        if conf.status in (DeviceStatus.ALIVE, DeviceStatus.MONITORING):
            get_network().onGetDeviceConfiguration(conf)

        # Clear the configuration editor
        # NOTE: This also clears configuration.descriptor!
        clear_configuration_instance(conf)

        # Add configuration with schema to device data
        conf.setSchema(schema)

        return conf

    def instance_gone(self, instance_id, instance_type):
        """A device or server was removed
        """
        devices, servers = [], []

        # Get the attributes of the instance which is now gone
        path = instance_type + '.' + instance_id
        attributes = self.get_attributes(path)
        if attributes is None:
            return devices, servers

        # Remove instance from system hash
        if self._system_hash is not None and path in self._system_hash:
            del self._system_hash[path]

        if instance_type in ('device', 'macro'):
            # Set the Configuration to offline, but keep it.
            device = self._online_devices.get(instance_id)
            if device is not None:
                device.status = DeviceStatus.OFFLINE
                clear_configuration_instance(device)

            # Update system tree
            self.system_tree.remove_device(instance_id)

            # Note the details of what device is gone
            class_id = attributes.get('classId', 'unknown-class')
            devices.append((instance_id, class_id, DeviceStatus.OFFLINE))

        elif instance_type == 'server':
            # Clear configuration parameter pages for the server's classes
            server_class_keys = self.system_tree.remove_server(instance_id)
            for key in server_class_keys:
                configuration = self._class_configurations.get(key)
                clear_configuration_instance(configuration)

            # Update the status of all devices
            for dev in self._online_devices.values():
                dev.updateStatus()

            # Note the details of what device is gone
            host = attributes.get('host', 'UNKNOWN')
            servers.append((instance_id, host, DeviceStatus.OFFLINE))

        return devices, servers

    def instance_new(self, server_hash):
        """Called when an `instanceNew` message is received from the server
        """
        remove_existing = self.system_tree.clear_existing
        existing_devices, server_class_keys = remove_existing(server_hash)

        # Clear configuration parameter pages, if existent
        for dev_id in existing_devices:
            configuration = self._online_devices.get(dev_id)
            clear_configuration_instance(configuration)

        for key in server_class_keys:
            configuration = self._class_configurations.get(key)
            clear_configuration_instance(configuration)

        # Update system topology with new configuration
        self.instance_updated(server_hash)

        return existing_devices

    def instance_updated(self, server_hash):
        """Called when an `instanceUpdated` message is received from the server
        """
        # Keep the system Hash up to date
        self.update(server_hash)

        # Topology changed so send new class schema requests
        if 'server' in server_hash:
            # Request schema for already viewed classes, if a server is new
            for server_id, class_id in self._class_configurations.keys():
                self.get_class(server_id, class_id)

    def update(self, server_hash):
        """A new device or server was added, or an existing device or server
        was updated with new information
        """
        if self._system_hash is None:
            self._system_hash = server_hash
        else:
            self._system_hash.merge(server_hash, "merge")

        # Update high-level representation
        new_topology_nodes = self.system_tree.update(server_hash)

        for dev in self._online_devices.values():
            dev.updateStatus()

            # Keep a reference to the topology node of the device
            if dev.id in new_topology_nodes:
                dev.topology_node = new_topology_nodes[dev.id]

    def update_alarms_info(self, alarm_data):
        """Update the ``SystemTreeNode`` objects with the current alarm types
        """
        update_types = alarm_data.get('update_types')
        alarm_entries = alarm_data.get('alarm_entries')

        def visitor(node):
            if node.attributes.get('type') != 'device':
                return

            for up_type, alarm_entry in zip(update_types, alarm_entries):
                if node.node_id == alarm_entry.deviceId:
                    if up_type in ADD_ALARM_TYPES:
                        node.append_alarm_type(alarm_entry.property,
                                               alarm_entry.type)
                    elif up_type in REMOVE_ALARM_TYPES:
                        node.remove_alarm_type(alarm_entry.property,
                                               alarm_entry.type)

        self.visit_system_tree(visitor)
        # NOTE: this should actually be called in the system_tree itself
        self.system_tree.needs_update = True
