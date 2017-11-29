#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 10, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from traits.api import (HasStrictTraits, Bool, Dict, Instance, Property,
                        on_trait_change)

from karabo.common.api import DeviceStatus
from karabo.middlelayer import Hash
from karabogui.alarms.api import ADD_ALARM_TYPES, REMOVE_ALARM_TYPES
from karabogui.binding.api import (
    BindingRoot, DeviceClassProxy, DeviceProxy, ProjectDeviceProxy,
    apply_configuration, apply_default_configuration, build_binding
)
from .project_device import ProjectDeviceInstance
from .tree import SystemTree


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

    # Mapping of (server id, class id) -> DeviceClassProxy
    _class_proxies = Dict

    # Mapping of (server id, class id) -> Schema
    _class_schemas = Dict

    # Mapping of device_id -> Hash
    _device_configurations = Dict

    # Mapping of device_id -> DeviceProxy
    _device_proxies = Dict

    # Mapping of device_id -> Schema
    _device_schemas = Dict

    # Mapping of device_id -> ProjectDeviceInstance
    _project_devices = Dict

    # Mapping of (server id, class id) -> {device_id: ProjectDeviceProxy}
    _project_device_proxies = Dict

    # A Hash instance holding the entire current topology
    _system_hash = Instance(Hash, allow_none=True)

    # Required by the singleton system (see ``karabogui.singleton.api``)
    def __init__(self, parent=None):
        super(SystemTopology, self).__init__()

    def clear(self):
        """Clear all saved devices and classes
        """
        self.clear_project_devices()
        self.system_tree.clear_all()

        self._system_hash = None
        self._class_proxies.clear()
        self._class_schemas.clear()
        self._device_configurations.clear()
        self._device_proxies.clear()
        self._device_schemas.clear()

    def clear_project_devices(self):
        """Called by project model on closing project
        """
        self._project_devices.clear()
        self._project_device_proxies.clear()

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
        """Return the proxy for a given class on a given server
        """
        key = (server_id, class_id)
        proxy = self._class_proxies.get(key)
        if proxy is None:
            binding = BindingRoot(class_id=class_id)
            proxy = DeviceClassProxy(server_id=server_id, binding=binding)
            self._class_proxies[key] = proxy

            attrs = self._get_device_attributes(server_id)
            if attrs is not None:
                proxy.refresh_schema()

        return proxy

    def get_configuration(self, device_id):
        """Return the up-to-date remote configuration for a given `device_id`.

        NOTE: This is not necessarily the same as the configuration which
        the user sees, if they have made changes that are not yet applied to
        the remote device.
        """
        return self._device_configurations.get(device_id)

    def get_device(self, device_id):
        """Return the proxy for a given device
        """
        proxy = self._device_proxies.get(device_id)
        if proxy is None:
            binding = BindingRoot()
            proxy = DeviceProxy(device_id=device_id, binding=binding)
            self._device_proxies[device_id] = proxy

            attrs = self._get_device_attributes(device_id)
            if attrs is not None:
                proxy.refresh_schema()
                binding.class_id = attrs.get('classId', '')
                proxy.server_id = attrs.get('serverId', '')

            # Get the system topology node, if it's there
            node = self.system_tree.get_instance_node(device_id)
            if node:
                proxy.topology_node = node

        return proxy

    def get_project_device(self, device_id, server_id='', class_id='',
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

            instance = ProjectDeviceInstance(device_id, server_id, class_id)
            instance.set_project_config_hash(init_config)
            self._project_devices[device_id] = instance
        else:
            instance = self._project_devices[device_id]
            instance.rename(
                device_id=device_id, class_id=class_id, server_id=server_id
            )

        return self._project_devices[device_id]

    def get_project_device_proxy(self, device_id, server_id, class_id):
        """Return the project device proxy for a given class on a given server
        """
        key = (server_id, class_id)
        mapping = self._project_device_proxies.setdefault(key, {})
        proxy = mapping.get(device_id)
        if proxy is None:
            schema = self._class_schemas.get(key)
            binding = (BindingRoot(class_id=class_id) if schema is None
                       else build_binding(schema))
            proxy = ProjectDeviceProxy(device_id=device_id,
                                       server_id=server_id,
                                       binding=binding)
            mapping[device_id] = proxy

            # Only fetch the schema if it's not already cached and server
            # exists
            if schema is None:
                attrs = self._get_device_attributes(server_id)
                if attrs is not None:
                    proxy.refresh_schema()

        return proxy

    def get_schema(self, server_id, class_id):
        """Return the schema for a given device class on a server.
        """
        key = (server_id, class_id)
        return self._class_schemas.get(key)

    def remove_project_device_proxy(self, device_id, server_id, class_id):
        """Remove the project device proxy for a given instance.
        """
        key = (server_id, class_id)
        mapping = self._project_device_proxies.get(key, {})
        mapping.pop(device_id, None)
        if not mapping:
            self._project_device_proxies.pop(key, None)

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
        """
        key = (server_id, class_id)
        if (key not in self._class_proxies and
                key not in self._project_device_proxies):
            # We used to print 'Unrequested schema for classId {} arrived' here
            return None

        self._class_schemas[key] = schema
        if len(schema.hash) == 0:
            return

        proxies = []
        if key in self._class_proxies:
            proxies = [self._class_proxies[key]]
        proxies.extend(self._project_device_proxies.get(key, {}).values())

        for proxy in proxies:
            # If the class schema has already arrived in the past, then we
            # should take no further action. We don't expect the schema of a
            # class on a running server to change. Device _instances_, however,
            # will change, but anyhow don't arrive in this handler.
            if len(proxy.binding.value) > 0:
                continue
            build_binding(schema, existing=proxy.binding)
            apply_default_configuration(proxy.binding)

    def device_config_updated(self, device_id, config):
        """Called when a `deviceConfiguration` message is received from the
        server

        Returns the ``DeviceProxy`` object associated with `device_id`, if
        one exists and is initialized.
        """
        proxy = self._device_proxies.get(device_id, None)
        if proxy is None or len(proxy.binding.value) == 0:
            return None

        self._device_configurations[device_id] = config

        # Apply the configuration to the proxy.
        # Leave modified values untouched!
        # XXX: Suppress traits notifications?
        apply_configuration(config, proxy.binding, skip_modified=True)
        # Fire the config_update event
        proxy.config_update = True

        return proxy

    def device_schema_updated(self, device_id, schema):
        """Called when a `deviceSchema` message is received from the server

        Returns the ``DeviceProxy`` object associated with `schema`, if one
        exists and is initialized.
        """
        proxy = self._device_proxies.get(device_id)
        if proxy is None:
            # We used to print 'Unrequested schema for device {} arrived' here
            return None

        self._device_schemas[device_id] = schema

        # XXX: We used to clear the configuration tree widget here

        # Rebuild the binding object
        build_binding(schema, existing=proxy.binding)

        return proxy

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
            # Set the DeviceProxy to offline, but keep it.
            proxy = self._device_proxies.get(instance_id)
            if proxy is not None:
                proxy.status = DeviceStatus.OFFLINE
                # XXX: We used to clear the configuration tree widget here

            # Update system tree
            self.system_tree.remove_device(instance_id)

            # Use pop() for removal in case there's nothing there
            self._device_schemas.pop(instance_id, None)
            self._device_configurations.pop(instance_id, None)

            # Note the details of what device is gone
            class_id = attributes.get('classId', 'unknown-class')
            devices.append((instance_id, class_id, 'offline'))

        elif instance_type == 'server':
            self.system_tree.remove_server(instance_id)
            # XXX: We used to clear all class tree widgets here

            # Update the status of all devices
            self._update_online_device_status()

            # Note the details of what device is gone
            host = attributes.get('host', 'UNKNOWN')
            servers.append((instance_id, host, 'offline'))

        return devices, servers

    def instance_new(self, server_hash):
        """Called when an `instanceNew` message is received from the server
        """
        existing_devices = self.system_tree.clear_existing(server_hash)

        # XXX: Clear device and class tree widgets, if existent

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
            for server_id, class_id in self._class_proxies.keys():
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

        self._update_online_device_status()

        # Update proxy topology node references
        for proxy in self._device_proxies.values():
            if proxy.device_id in new_topology_nodes:
                proxy.topology_node = new_topology_nodes[proxy.device_id]

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

    # ---------------------------------------------------------------------
    # Utilities

    def _get_device_attributes(self, device_id):
        """Get the attributes for a device, if it's online. Otherwise get None.
        """
        attrs = None
        for parent in ('device', 'macro', 'server'):
            attrs = self.get_attributes(parent + '.' + device_id)
            if attrs is not None:
                break
        return attrs

    def _update_online_device_status(self):
        """Check all device proxies to see if they are online or not
        """
        for dev in self._device_proxies.values():
            attrs = self._get_device_attributes(dev.device_id)
            if dev.status is DeviceStatus.OFFLINE and attrs:
                dev.status = DeviceStatus.ONLINE
            elif dev.status is not DeviceStatus.OFFLINE and attrs is None:
                dev.status = DeviceStatus.OFFLINE
