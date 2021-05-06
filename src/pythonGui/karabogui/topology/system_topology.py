#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 10, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import (
    Bool, Dict, HasStrictTraits, Instance, Property, Set, on_trait_change)

from karabo.common.api import ProxyStatus
from karabo.native import Hash
from karabogui.alarms.api import ADD_ALARM_TYPES, REMOVE_ALARM_TYPES
from karabogui.binding.api import (
    BindingRoot, DeviceClassProxy, DeviceProxy, ProjectDeviceProxy,
    apply_configuration, apply_default_configuration, build_binding)

from .device_tree import DeviceSystemTree
from .project_device import ProjectDeviceInstance
from .system_tree import SystemTree


class SystemTopology(HasStrictTraits):
    """The Karabo System Topology

    This is a data structure containing all of the online devices and device
    servers in a running Karabo system. It is kept continually up-to-date by
    the connection to the GUI server.
    """

    # A high-level version of `_system_hash`. Used by GUI views.
    system_tree = Instance(SystemTree)

    # A high-level version for device folded structure
    device_tree = Instance(DeviceSystemTree)

    # True if the system is online, False otherwise
    online = Property(Bool)

    # Mapping of (server id, class id) -> DeviceClassProxy
    _class_proxies = Dict

    # Mapping of (server id, class id) -> Schema
    _class_schemas = Dict

    # Tracking of requested Class Schemas for (server id, class id)
    _requested_class_schemas = Set

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
        self.device_tree.clear_all()

        self._system_hash = None
        self._class_proxies.clear()
        self._class_schemas.clear()
        self._device_proxies.clear()
        self._device_schemas.clear()
        self._requested_class_schemas = set()

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
        """Return the class proxy (DeviceClassProxy) for a given
        class on a given server
        """
        key = (server_id, class_id)
        proxy = self._class_proxies.get(key)
        if proxy is None:
            binding = BindingRoot(class_id=class_id)
            proxy = DeviceClassProxy(server_id=server_id, binding=binding)
            self._class_proxies[key] = proxy

            attrs = self._get_device_attributes(server_id)
            if attrs and class_id in attrs.get('deviceClasses', ()):
                # NOTE: The server is online and has the device class we want
                # but we only request if it was not previously requested!
                request = key not in self._requested_class_schemas
                if request:
                    self._requested_class_schemas.add(key)
                    proxy.refresh_schema()
                else:
                    proxy.status = ProxyStatus.REQUESTED

        return proxy

    def get_device(self, device_id):
        """Return the online version proxy (DeviceProxy) for a given device
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

        The ProjectDeviceInstance holds two proxies, when the device is online
        it uses DeviceProxy, when the device is offline, it uses
        ProjectDeviceProxy (which is a modified version of ClassDeviceProxy)
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
        attrs = self._get_device_attributes(device_id)
        if attrs is not None:
            instance.error = (attrs.get('status', '') == 'error')

        return self._project_devices[device_id]

    def delete_project_device(self, device_id):
        return self._project_devices.pop(device_id, None)

    def get_project_device_proxy(self, device_id, server_id, class_id,
                                 create_instance=True):
        """Return the project device proxy (offline version of a device in a
        certain project) for a given class on a given server

        If `create_instance` is False, avoid creating the project device and
        return None.
        """
        key = (server_id, class_id)
        mapping = self._project_device_proxies.setdefault(key, {})
        proxy = mapping.get(device_id)
        if proxy is None and create_instance:
            schema = self._class_schemas.get(key)
            binding = (BindingRoot(class_id=class_id) if schema is None
                       else build_binding(schema))
            proxy = ProjectDeviceProxy(device_id=device_id,
                                       server_id=server_id,
                                       binding=binding)
            mapping[device_id] = proxy

            # Only fetch the schema if it's not already cached and server
            # exists and device plugin is installed
            if schema is None:
                if proxy.status not in (ProxyStatus.NOSERVER,
                                        ProxyStatus.NOPLUGIN):
                    # We only request if it was not previously requested!
                    request = key not in self._requested_class_schemas
                    if request:
                        self._requested_class_schemas.add(key)
                        proxy.refresh_schema()
                    else:
                        proxy.status = ProxyStatus.REQUESTED
                else:
                    # The device class is not installed on the server, but
                    # requested from the project, we create an empty
                    # DeviceClassProxy in the system topology and wait for
                    # a server restart. Once the device plugin is installed
                    # on the server, requesting schema and building bindings
                    # will be triggered automatically
                    self.get_class(*key)

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

    def visit_device_tree(self, visitor):
        """Walk every node in the device tree and run a `visitor` function on
        each item.
        """
        self.device_tree.visit(visitor)

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

    def _device_tree_default(self):
        """We must provide a default initializer since the singleton system
        will instantiate the once instance of this object.
        """
        return DeviceSystemTree()

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

        if len(schema.hash) > 0:
            # if it's a valid schema we cache it even if it is not requested,
            # may be useful in future
            self._class_schemas[key] = schema

        if (key not in self._class_proxies.keys() and
                key not in self._project_device_proxies.keys()):
            # This schema has nothing to do with us whatsoever
            # We used to print 'Unrequested schema for classId {} arrived'.
            return

        if key in self._requested_class_schemas:
            self._requested_class_schemas.remove(key)

        proxies = []
        if key not in self._class_proxies.keys():
            # We lazy build a DeviceClassProxy for future usage
            binding = build_binding(schema)
            proxy = DeviceClassProxy(server_id=server_id, binding=binding)
            self._class_proxies[key] = proxy
            apply_default_configuration(proxy.binding)
        else:
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
            if proxy is self._class_proxies.get(key, None):
                # Only apply default config if not a project device!
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

        # Apply the configuration to the proxy.
        apply_configuration(config, proxy.binding)
        # Fire the config_update event
        # XXX: Still needed with proxy.binding.config_update?
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
                proxy.status = ProxyStatus.OFFLINE
                # XXX: We used to clear the configuration tree widget here

            # Update system tree
            self.system_tree.remove_device(instance_id)

            self.device_tree.remove_device(instance_id)

            # Use pop() for removal in case there's nothing there
            self._device_schemas.pop(instance_id, None)

            # Note the details of what device is gone
            class_id = attributes.get('classId', 'unknown-class')
            devices.append((instance_id, class_id, 'offline'))

        elif instance_type == 'server':
            self.system_tree.remove_server(instance_id)
            for key in self._class_proxies.keys():
                if key[0] == instance_id:
                    # Clear the class proxy binding values when device server
                    # goes offline
                    self._class_proxies[key].binding.value.clear_namespace()

            # Update the status of all online devices (device proxies)
            self._update_online_device_status()

            # Update the status of all offline devices (project device proxies)
            self._project_device_proxies_server_update(instance_id)

            # Note the details of what device is gone
            host = attributes.get('host', 'UNKNOWN')
            servers.append((instance_id, host, 'offline'))

        return devices, servers

    def instance_new(self, server_hash):
        """Called when an `instanceNew` message is received from the server
        """
        existing_devices = self.system_tree.clear_existing(server_hash)
        # Update system topology with new configuration
        self.update(server_hash)

        # Topology changed so send new class schema requests
        srv_hsh = server_hash.get('server', None)
        if srv_hsh is None:
            return

        for server_id in srv_hsh:
            for srv_id, cls_id in self._class_proxies.keys():
                if (srv_id == server_id and cls_id
                        in srv_hsh[srv_id, ...].get('deviceClasses', ())):
                    proxy = self._class_proxies[(srv_id, cls_id)]
                    if len(proxy.binding.value) == 0:
                        proxy.refresh_schema()

            # Update all offline devices (project device proxies)
            self._project_device_proxies_server_update(server_id)

        return existing_devices

    def instance_updated(self, server_hash):
        """Called when an `instanceUpdated` message is received from the server
        """
        if self._system_hash is None:
            self._system_hash = server_hash
        else:
            self._system_hash.merge(server_hash, "merge")

        # Only the system tree takes an instance update at the moment
        # for the instanceInfo
        self.system_tree.instance_update(server_hash)
        # Note: Check if this is really needed!
        self._update_online_device_status()

        # Topology changed so send new class schema requests
        srv_hsh = server_hash.get('server', None)
        if srv_hsh is None:
            return

        for server_id in srv_hsh:
            for srv_id, cls_id in self._class_proxies.keys():
                if (srv_id == server_id and cls_id
                        in srv_hsh[srv_id, ...].get('deviceClasses', ())):
                    proxy = self._class_proxies[(srv_id, cls_id)]
                    if len(proxy.binding.value) == 0:
                        proxy.refresh_schema()

            # Update all offline devices (project device proxies)
            self._project_device_proxies_server_update(server_id)

    def initialize(self, server_hash):
        """Initialize the system topology with the system hash"""
        self._system_hash = server_hash
        self.system_tree.initialize(server_hash)
        self.device_tree.initialize(server_hash)

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

        self.device_tree.update(server_hash)
        for proxy in self._device_proxies.values():
            attrs = self._get_device_attributes(proxy.device_id)
            if proxy.status is ProxyStatus.OFFLINE and attrs:
                proxy.status = ProxyStatus.ONLINE
            elif proxy.status is not ProxyStatus.OFFLINE and attrs is None:
                proxy.status = ProxyStatus.OFFLINE
            project_device = self._project_devices.get(proxy.device_id)
            if project_device is not None and attrs is not None:
                project_device.error = (attrs.get('status', '') == 'error')

            if proxy.device_id in new_topology_nodes:
                proxy.topology_node = new_topology_nodes[proxy.device_id]
                proxy.server_id = attrs.get('serverId', '')

    def update_alarms_info(self, alarm_data):
        """Update the ``SystemTreeNode`` objects with the current alarm types
        """
        update_types = alarm_data.get('update_types')
        alarm_entries = alarm_data.get('alarm_entries')
        # Only update system tree if the maximum alarm level has changed for
        # any device, avoiding unnecessary repaint of the system tree.
        needs_update = False

        node_ids = set()
        for up_type, alarm_entry in zip(update_types, alarm_entries):
            node = self.system_tree.get_instance_node(alarm_entry.deviceId)
            if node is not None:
                if up_type in ADD_ALARM_TYPES:
                    needs_update |= node.append_alarm_type(
                        alarm_entry.property, alarm_entry.type)
                elif up_type in REMOVE_ALARM_TYPES:
                    needs_update |= node.remove_alarm_type(
                        alarm_entry.property, alarm_entry.type)

                # add nodes for event firing!
                node_ids.add(alarm_entry.deviceId)
        # NOTE: this should actually be called in the system_tree itself
        if needs_update:
            self.system_tree.alarm_update = node_ids

    # ---------------------------------------------------------------------
    # Topology interface

    def topology_update(self, changes):
        """Handle the bulk system topology update from the server

        This function will return the devices and servers which have been
        removed from the topology via ``slotInstanceGone``
        """
        devices, servers = [], []

        gone = changes['gone']
        if not gone.empty():
            devices, servers = self.topology_gone(gone)
        new = changes['new']
        if not new.empty():
            self.instance_new(new)
        update = changes['update']
        if not update.empty():
            self.instance_updated(update)

        return devices, servers

    def topology_gone(self, system_hash):
        """Remove servers, devices and macros from the system topology

        This function will return the devices and servers which have been
        removed from the topology in separate lists ``devices`` and ``servers``
        """
        devices, servers = [], []

        if 'device' in system_hash:
            self.topology_device_gone('device', system_hash, devices)
        if 'macro' in system_hash:
            self.topology_device_gone('macro', system_hash, devices)
        if 'server' in system_hash:
            for instance_id in system_hash['server'].keys():
                path = 'server' + '.' + instance_id
                attributes = self.get_attributes(path)
                if attributes is None:
                    continue

                # Remove instance from system hash
                if self._system_hash is not None and path in self._system_hash:
                    del self._system_hash[path]

                self.system_tree.remove_server(instance_id)
                for key in self._class_proxies.keys():
                    if key[0] == instance_id:
                        # Clear the class proxy binding values when device
                        # server goes offline
                        proxy = self._class_proxies[key]
                        proxy.binding.value.clear_namespace()
                # Update status of all offline project device proxies
                self._project_device_proxies_server_update(instance_id)

                # Note the details of what device is gone
                host = attributes.get('host', 'UNKNOWN')
                servers.append((instance_id, host, 'offline'))

            # Update the status of all online devices (device proxies)
            self._update_online_device_status()

        return devices, servers

    def topology_device_gone(self, instance_type, system_hash, devices):
        """Check if devices or macros are gone in the system topology

        :param devices: Extended ist of devices and macros for which are
                        removed from the topology
        """
        for instance_id, _, attr in system_hash[instance_type].iterall():
            # Get the attributes of the instance which is now gone
            path = instance_type + '.' + instance_id
            attributes = self.get_attributes(path)
            if attributes is None:
                continue

            # Remove instance from system hash
            if self._system_hash is not None and path in self._system_hash:
                del self._system_hash[path]

            # Set the DeviceProxy to offline, but keep it.
            proxy = self._device_proxies.get(instance_id)
            if proxy is not None:
                proxy.status = ProxyStatus.OFFLINE
                # XXX: We used to clear the configuration tree widget here

            # Update system tree
            self.system_tree.remove_device(instance_id)

            # And the device tree
            self.device_tree.remove_device(instance_id)

            # Use pop() for removal in case there's nothing there
            self._device_schemas.pop(instance_id, None)

            # Note the details of what device is gone
            class_id = attributes.get('classId', 'unknown-class')
            devices.append((instance_id, class_id, 'offline'))

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
        """Check all online version of device proxies (DeviceProxy or
        ProjectDevice) to see if their status should be updated.

        Call this function every time a new system hash is arrived from the
        GUI server, so we can keep all device presentations up to date
        """
        for proxy in self._device_proxies.values():
            device_id = proxy.device_id
            # extract this device's information from system hash
            attrs = self._get_device_attributes(device_id)
            if proxy.status is ProxyStatus.OFFLINE and attrs:
                proxy.status = ProxyStatus.ONLINE
            elif proxy.status is not ProxyStatus.OFFLINE and attrs is None:
                proxy.status = ProxyStatus.OFFLINE
            # Synchronize project errors
            project_device = self._project_devices.get(device_id)
            if project_device is not None and attrs is not None:
                project_device.error = (attrs.get('status', '') == 'error')

    def _project_device_proxies_server_update(self, server_id):
        """Because the server online status has changed , update the project
        device proxy's status accordingly
        """
        for (srv_id, cls_id), mapping in self._project_device_proxies.items():
            # key -> (server_id, class_id)
            # mapping -> {deviceId: project_device_proxy}
            if server_id == srv_id:
                for dev_id, proxy in mapping.items():
                    proxy.update_status()
