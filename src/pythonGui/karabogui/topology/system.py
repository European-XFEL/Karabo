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
            if attrs is not None:
                proxy.refresh_schema()

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
                if proxy.status not in (DeviceStatus.NOSERVER,
                                        DeviceStatus.NOPLUGIN):
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
                proxy.status = DeviceStatus.OFFLINE
                # XXX: We used to clear the configuration tree widget here

            # Update system tree
            self.system_tree.remove_device(instance_id)

            # Use pop() for removal in case there's nothing there
            self._device_schemas.pop(instance_id, None)

            # Note the details of what device is gone
            class_id = attributes.get('classId', 'unknown-class')
            devices.append((instance_id, class_id, 'offline'))

        elif instance_type == 'server':
            self.system_tree.remove_server(instance_id)
            # XXX: We used to clear all class tree widgets here

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
        srv_hsh = server_hash.get('server', None)
        if srv_hsh is not None:
            # Request schema for already viewed classes, if a server is new
            for server_id, class_id in self._class_proxies.keys():
                if server_id in srv_hsh:
                    self.get_class(server_id, class_id)

            # Update all offline devices (project device proxies)
            for server_id in srv_hsh:
                self._project_device_proxies_server_update(server_id)

    def update(self, server_hash):
        """A new device or server was added, or an existing device or server
        was updated with new information
        """
        if self._system_hash is None:
            self._system_hash = server_hash
        else:
            self._system_hash.merge(server_hash, "merge")

        # Update high-level representation
        # Error and alarm information are also get updated to nodes appear in
        # the system topology in the navigation panel
        new_topology_nodes = self.system_tree.update(server_hash)

        self._update_online_device_status()

        # Update proxy topology node references
        for proxy in self._device_proxies.values():
            if proxy.device_id in new_topology_nodes:
                proxy.topology_node = new_topology_nodes[proxy.device_id]
                attrs = self._get_device_attributes(proxy.device_id)
                proxy.server_id = attrs.get('serverId', '')

    def update_alarms_info(self, alarm_data):
        """Update the ``SystemTreeNode`` objects with the current alarm types
        """
        update_types = alarm_data.get('update_types')
        alarm_entries = alarm_data.get('alarm_entries')
        # Only update system tree if the maximum alarm level has changed for
        # any device, avoiding unnecessary repaint of the system tree.
        needs_update = False

        for up_type, alarm_entry in zip(update_types, alarm_entries):
            node = self.system_tree.get_instance_node(alarm_entry.deviceId)
            if node is not None:
                if up_type in ADD_ALARM_TYPES:
                    needs_update |= node.append_alarm_type(
                        alarm_entry.property, alarm_entry.type)
                elif up_type in REMOVE_ALARM_TYPES:
                    needs_update |= node.remove_alarm_type(
                        alarm_entry.property, alarm_entry.type)

        # NOTE: this should actually be called in the system_tree itself
        if needs_update:
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
        """Check all online version of device proxies (DeviceProxy or
        ProjectDevice) to see if their status should be updated.

        Call this function every time a new system hash is arrived from the
        GUI server, so we can keep all device presentations up to date
        """
        # self._device_proxies is a dictionary keeps all the online devices
        # which are visible in the topology
        for dev in self._device_proxies.values():
            did = dev.device_id
            # extract this device's information from system hash
            attrs = self._get_device_attributes(did)
            if dev.status is DeviceStatus.OFFLINE and attrs:
                dev.status = DeviceStatus.ONLINE
            elif dev.status is not DeviceStatus.OFFLINE and attrs is None:
                dev.status = DeviceStatus.OFFLINE
            # The information we pull out of the system hash may also contain
            # device error or alarm information, project device will not be
            # updated until user show them in the configurator, so here we set
            # the shortcut flag directly so their icon will be updated in the
            # project panel view.
            prj_dev = self._project_devices.get(did)
            if prj_dev is not None and attrs is not None:
                prj_dev.error = (attrs.get('status', '') == 'error')

    def _project_device_proxies_server_update(self, server_id):
        """Check if the server_id and class_id of a project device proxy is in
        the system topology, update the status accordingly
        """
        for (srv_id, cls_id), mapping in self._project_device_proxies.items():
            # mapping -> {deviceId: project_device_proxy}
            if server_id != srv_id:
                continue
            path = 'server.{}'.format(srv_id)
            attributes = self.get_attributes(path)
            for proxy in mapping.values():
                if attributes is None:
                    # This proxy's server is not found in the system topology
                    proxy.status = DeviceStatus.NOSERVER
                else:
                    # Get all available device classes on this server
                    dev_classes = attributes.get('deviceClasses', [])
                    if cls_id in dev_classes:
                        # Only change the proxy status if its previously not
                        # available (i.e. noserver or noplugin)
                        if proxy.status in (DeviceStatus.NOSERVER,
                                            DeviceStatus.NOPLUGIN):
                            proxy.status = DeviceStatus.OFFLINE
                    else:
                        # We know the server is online, but no device class
                        proxy.status = DeviceStatus.NOPLUGIN
