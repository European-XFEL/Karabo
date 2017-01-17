#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 10, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from traits.api import HasStrictTraits, Bool, Dict, Event, Instance, Property

from karabo.middlelayer import Hash, Schema
from karabo_gui.configuration import BulkNotifications, Configuration
from karabo_gui.singletons.api import get_network
from karabo_gui.system_tree import SystemTree


class ProjectDeviceInstance(HasStrictTraits):
    """An abstraction of devices needed for projects and workflow design.
    No matter the online/offline state of a device, you can get a
    ``Configuration`` object which is valid and associated with the same
    device id.
    """
    # The current configuration for this device
    current_configuration = Property(Instance(Configuration))

    # An event which is triggered whenever the configuration is updated
    configuration_updated = Event

    # Binary online/offline state of the device
    online = Bool

    # The online/offline configurations
    _class_config = Instance(Configuration)
    _project_device = Instance(Configuration)
    _real_device = Instance(Configuration)

    def __init__(self, real_device, project_device, class_config):
        super(ProjectDeviceInstance, self).__init__()

        self.online = real_device.isOnline()
        self._class_config = class_config
        self._project_device = project_device
        self._real_device = real_device

        # Connect to signals
        class_config.signalNewDescriptor.connect(self._descriptor_change_slot)
        real_device.signalStatusChanged.connect(self._status_change_slot)

        if class_config.descriptor is not None:
            self._descriptor_change_slot(class_config)

    def destroy(self):
        """Disconnect slots which are connected to this object's methods
        """
        new_descriptor = self._descriptor_change_slot
        self._class_config.signalNewDescriptor.disconnect(new_descriptor)
        status_changed = self._status_change_slot
        self._real_device.signalStatusChanged.disconnect(status_changed)

    def _get_current_configuration(self):
        """Traits Property getter for the current configuration
        """
        if self.online:
            return self._real_device

        return self._project_device

    def _descriptor_change_slot(self, config):
        """The global class has received a new schema which needs to be set
        for the dependent device ``Configuration`` instances.
        """
        if self._project_device.descriptor is not None:
            self._project_device.redummy()
        self._project_device.descriptor = config.descriptor

        if self._real_device.descriptor is not None:
            self._real_device.redummy()
        self._real_device.descriptor = config.descriptor

        # Set default values for offline configuration
        self._project_device.setDefault()

        # Let the world know
        self.configuration_updated = True

    def _status_change_slot(self, status, error_flag):
        """The `_real_device` has changed its status. Check if it's online
        """
        self.online = self._real_device.isOnline()

        # Let the world know
        self.configuration_updated = True


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
    _offline_devices = Dict

    # Mapping of device_id -> ProjectDeviceConfiguration
    _project_devices = Dict

    # A Hash instance holding the entire current topology
    _system_hash = Instance(Hash, allow_none=True)

    # Required by the singleton system (see ``karabo_gui.singleton.api``)
    def __init__(self, parent=None):
        super(SystemTopology, self).__init__()

    def clear(self):
        """Clear all saved devices and classes
        """
        for dev in self._project_devices.values():
            dev.destroy()

        self.system_tree.clear_all()
        self._system_hash = None
        self._class_schemas = {}
        self._class_configurations = {}
        self._online_devices = {}
        self._offline_devices = {}
        self._project_devices = {}

    def get_attributes(self, topology_path):
        """Return the attributes of a given node in the `_system_hash`.
        """
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
            self._class_configurations[key] = klass

        statuses = ('requested', 'schema')
        if klass.descriptor is None or klass.status not in statuses:
            get_network().onGetClassSchema(*key)
            klass.status = 'requested'

        return klass

    def get_device(self, device_id):
        """Return the online configuration for a given device
        """
        device = self._online_devices.get(device_id)
        if device is None:
            device = Configuration(device_id, 'device')
            self._online_devices[device_id] = device
            device.updateStatus()

        statuses = ('offline', 'requested')
        if device.descriptor is None and device.status not in statuses:
            get_network().onGetDeviceSchema(device_id)
            device.status = 'requested'

        return device

    def get_project_device(self, device_id, class_id='', server_id=''):
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

            offline_device = Configuration(device_id, 'projectClass')
            offline_device.serverId = server_id
            offline_device.classId = class_id
            self._offline_devices[device_id] = offline_device

            class_configuration = self.get_class(server_id, class_id)
            online_device = self.get_device(device_id)
            instance = ProjectDeviceInstance(online_device, offline_device,
                                             class_configuration)
            self._project_devices[device_id] = instance

        return self._project_devices[device_id]

    def get_schema(self, server_id, class_id):
        """Return the ``Schema`` for a given class on a given server
        """
        key = (server_id, class_id)
        return self._class_schemas.get(key)

    def search_system_tree(self, root_path, filter):
        """Iterate over part of the `_system_hash` Hash and run a `filter`
        function on each item.

        Returns a list of instance IDs for each item where the `filter`
        function returned `True`.
        """
        instance_ids = []
        if root_path in self._system_hash:
            for inst_id, _, attrs in self._system_hash[root_path].iterall():
                if filter(inst_id, attrs):
                    instance_ids.append(inst_id)
        return instance_ids

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
        device = self._online_devices[device_id]
        if device is None or device.descriptor is None:
            return None

        with BulkNotifications(device):
            device.fromHash(config)

        if device.status == 'schema':
            device.status = 'alive'
        if device.status == 'alive' and device.visible > 0:
            device.status = 'monitoring'

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
        if conf.status in ('alive', 'monitoring'):
            get_network().onGetDeviceConfiguration(conf)

        # Add configuration with schema to device data
        conf.setSchema(schema)

        return conf

    def instance_gone(self, instance_id, instance_type):
        """A device or server was removed
        """
        devices, servers = [], []

        # Get the attributes of the instance which is now gone
        path = instance_type + '.' + instance_id
        try:
            attributes = self._system_hash[path, ...]
        except KeyError:
            attributes = {}

        if instance_type in ('device', 'macro'):
            # Set the Configuration to offline, but keep it.
            device = self._online_devices.get(instance_id)
            if device is not None:
                device.status = 'offline'
                _clear_configuration_instance(device)

            # Update system tree
            self.system_tree.remove_device(instance_id)

            # Note the details of what device is gone
            class_id = attributes.get('classId', 'unknown-class')
            devices.append((instance_id, class_id, 'offline'))

        elif instance_type == 'server':
            # Clear configuration parameter pages for the server's classes
            server_class_keys = self.system_tree.remove_server(instance_id)
            for key in server_class_keys:
                configuration = self._class_configurations.get(key)
                _clear_configuration_instance(configuration)

            # Update the status of all devices
            for dev in self._online_devices.values():
                dev.updateStatus()

            # Update system tree
            self.system_tree.remove_server(instance_id)

            # Note the details of what device is gone
            host = attributes.get('host', 'UNKNOWN')
            servers.append((instance_id, host, 'offline'))

        # Remove instance from system hash
        if self._system_hash is not None and path in self._system_hash:
            del self._system_hash[path]

        return devices, servers

    def instance_new(self, server_hash):
        """Called when an `instanceNew` message is received from the server
        """
        remove_existing = self.system_tree.clear_existing
        existing_devices, server_class_keys = remove_existing(server_hash)

        # Clear configuration parameter pages, if existent
        for dev_id in existing_devices:
            configuration = self._online_devices.get(dev_id)
            _clear_configuration_instance(configuration)

        for key in server_class_keys:
            configuration = self._class_configurations.get(key)
            _clear_configuration_instance(configuration)

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
        self.system_tree.update(server_hash)

        for dev in self._online_devices.values():
            dev.updateStatus()


def _clear_configuration_instance(configuration):
    """Clear some of the state built up in a Configuration object.
    """
    if configuration is None:
        return

    if configuration.parameterEditor is not None:
        configuration.parameterEditor.clear()

    if configuration.descriptor is not None:
        configuration.redummy()
