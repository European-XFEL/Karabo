#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 10, 2017
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from traits.api import (
    Bool, Dict, HasStrictTraits, Instance, Property, on_trait_change)

from karabo.native import Hash
from karabogui.binding.api import (
    NO_CLASS_STATUSES, BindingRoot, DeviceClassProxy, DeviceProxy,
    ProjectDeviceProxy, ProxyStatus, apply_configuration,
    apply_default_configuration, build_binding)

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

    # Mapping of server_id-> {class_id: Schema}
    _class_schemas = Dict

    # Tracking of requested Class Schemas for {server id: set()} -> class id
    _requested_classes = Dict

    # Mapping of device_id -> DeviceProxy
    _device_proxies = Dict

    # Mapping of device_id -> ProjectDeviceInstance
    _project_devices = Dict

    # Mapping of (server id, class id) -> {device_id: ProjectDeviceProxy}
    _project_device_proxies = Dict

    # A Hash instance holding the entire current topology
    _system_hash = Instance(Hash, args=())

    # Required by the singleton system (see ``karabogui.singleton.api``)
    def __init__(self, parent=None):
        super().__init__()

    def clear(self):
        """Clear all saved devices and classes"""
        self.clear_project_devices()
        self.system_tree.clear_all()
        self.device_tree.clear_all()

        self._system_hash.clear()
        self._class_proxies.clear()
        self._class_schemas.clear()
        self._device_proxies.clear()
        self._requested_classes.clear()

    def clear_project_devices(self):
        """Called by project model on closing project"""
        self._project_devices.clear()
        self._project_device_proxies.clear()

    def get_attributes(self, topology_path):
        """Return the attributes of a given node in the `_system_hash`"""
        try:
            return self._system_hash[topology_path, ...]
        except KeyError:
            return None

    def get_class(self, server_id, class_id):
        """Return the class proxy (DeviceClassProxy) for a given
        class on a given server

        This function will request the class schema if not available
        """
        key = (server_id, class_id)
        proxy = self._class_proxies.get(key)
        if proxy is None:
            binding = BindingRoot(class_id=class_id)
            proxy = DeviceClassProxy(server_id=server_id, binding=binding)
            self._class_proxies[key] = proxy
            path = 'server' + '.' + server_id
            attrs = self.get_attributes(path)
            if attrs and class_id in attrs.get('deviceClasses', ()):
                # NOTE: The server is online and has the device class we want
                # but we only request if it was not previously requested!
                request = class_id not in self._requested_classes.setdefault(
                    server_id, set())
                if request:
                    self._requested_classes[server_id].add(class_id)
                    proxy.refresh_schema()
                else:
                    proxy.status = ProxyStatus.REQUESTED

        return proxy

    def get_device(self, device_id, request=True):
        """Return the online version proxy (DeviceProxy) for a given device

        :param request: Request a schema for the freshly created online proxy
                        This is not required for lazy building, however, the
                        default is `True`.
        """
        proxy = self._device_proxies.get(device_id)
        if proxy is None:
            binding = BindingRoot()
            proxy = DeviceProxy(device_id=device_id, binding=binding)
            self._device_proxies[device_id] = proxy

            attrs = self._get_device_attributes(device_id)
            if attrs is not None:
                # We are dealing with an online device. Hence we assign
                # the class id and server id from the system hash and
                # if desired, request a schema.
                binding.class_id = attrs['classId']
                proxy.server_id = attrs['serverId']
                if request:
                    proxy.refresh_schema()
                elif proxy.status is ProxyStatus.OFFLINE:
                    # We did not request a schema, but we have been freshly
                    # created and are ONLINE
                    proxy.status = ProxyStatus.ONLINE
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
                device_id=device_id, class_id=class_id, server_id=server_id)

        return self._project_devices[device_id]

    def delete_project_device(self, device_id):
        return self._project_devices.pop(device_id, None)

    def get_project_device_proxy(self, device_id, server_id, class_id):
        """Return the project device proxy (offline version of a device in a
        certain project) for a given class on a given server

        Offline devices are always created lazily with an empty `BindingRoot`.
        """
        key = (server_id, class_id)
        mapping = self._project_device_proxies.setdefault(key, {})
        proxy = mapping.get(device_id)
        if proxy is None:
            binding = BindingRoot(class_id=class_id)
            proxy = ProjectDeviceProxy(device_id=device_id,
                                       server_id=server_id,
                                       binding=binding)
            mapping[device_id] = proxy

        return proxy

    def get_schema(self, server_id, class_id):
        """Return the schema for a given device class on a server.
        """
        classes = self._class_schemas.setdefault(server_id, {})
        return classes.get(class_id)

    def ensure_proxy_class_schema(self, device_id, server_id, class_id):
        """Ensure the class schema of a project device proxy if necessary"""
        key = (server_id, class_id)
        mapping = self._project_device_proxies.setdefault(key, {})
        proxy = mapping.get(device_id)
        if proxy is not None and not len(proxy.binding.value) > 0:
            schema = self.get_schema(server_id, class_id)
            if schema is not None:
                build_binding(schema, existing=proxy.binding)
            elif proxy.status not in NO_CLASS_STATUSES:
                # We only request if it was not previously requested!
                request = class_id not in self._requested_classes.setdefault(
                    server_id, set())
                if request:
                    self._requested_classes[server_id].add(class_id)
                    proxy.refresh_schema()
                else:
                    proxy.status = ProxyStatus.REQUESTED

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
        """The topology is considered online if `_system_hash` is not empty.
        """
        return not self._system_hash.empty()

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
        if len(schema.hash) > 0:
            # if it's a valid schema we cache it even if it is not requested,
            # may be useful in future
            self._class_schemas.setdefault(server_id, {})[class_id] = schema

        key = (server_id, class_id)
        if (key not in self._class_proxies.keys() and
                key not in self._project_device_proxies.keys()):
            # This schema has nothing to do with us whatsoever
            # We used to print 'Unrequested schema for classId {} arrived'.
            return

        self._requested_classes.setdefault(server_id, set()).discard(class_id)

        proxies = []
        if key not in self._class_proxies.keys():
            # We lazy build a DeviceClassProxy for future usage
            binding = build_binding(schema)
            proxy = DeviceClassProxy(server_id=server_id, binding=binding)
            self._class_proxies[key] = proxy
            apply_default_configuration(proxy.binding)
            proxy.config_update = True
        else:
            # Make sure we update our class proxies for the topology!
            proxies = [self._class_proxies[key]]

        proxies.extend(self._project_device_proxies.get(key, {}).values())
        for proxy in proxies:
            # 1. If the class schema has already arrived in the past, then we
            # should take no further action. We don't expect the schema of a
            # class on a running server to change. Device _instances_, however,
            # will change, but anyhow don't arrive in this handler.
            # 2. Only proxies are updated where we requested this!
            if (len(proxy.binding.value) > 0
                    or proxy.status != ProxyStatus.REQUESTED):
                continue
            build_binding(schema, existing=proxy.binding)
            if proxy is self._class_proxies.get(key, None):
                # Only apply default config if not a project device!
                apply_default_configuration(proxy.binding)
                proxy.config_update = True

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

        # Rebuild the binding object
        build_binding(schema, existing=proxy.binding)

        return proxy

    def initialize(self, server_hash):
        """Initialize the system topology with the system hash"""
        self._system_hash = server_hash
        self.system_tree.initialize(server_hash)
        self.device_tree.initialize(server_hash)

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
            devices, servers = self.instance_gone(gone)
        new = changes['new']
        if not new.empty():
            self.instance_new(new)
        update = changes['update']
        if not update.empty():
            self.instance_updated(update)

        return devices, servers

    def instance_new(self, server_hash):
        """Called when an `instanceNew` message is received from the server
        """
        existing_devices = self.system_tree.clear_existing(server_hash)
        # Update system topology with new configuration

        self._system_hash.merge(server_hash, "merge")

        # Update the high-level representation (Trees)
        new_topology_nodes = self.system_tree.update(server_hash)
        self.device_tree.update(server_hash)

        # Attach topology nodes and set the status
        for deviceId, node in new_topology_nodes.items():
            proxy = self._device_proxies.get(deviceId)
            if proxy is not None:
                proxy.topology_node = node
                # Set the fundamental info of the proxy
                proxy.server_id = node.attributes["serverId"]
                proxy.binding.class_id = node.attributes["classId"]
                proxy.status = ProxyStatus.ONLINE

        self._request_server_classes(server_hash)

        return existing_devices

    def instance_updated(self, server_hash):
        """Called when an `instanceUpdated` message is received from the server
        """
        self._system_hash.merge(server_hash, "merge")

        # Only the system tree takes an instance update at the moment
        # for the instanceInfo
        self.system_tree.instance_update(server_hash)
        # And the device tree!
        self.device_tree.instance_update(server_hash)

        self._request_server_classes(server_hash)

    def instance_gone(self, system_hash):
        """Remove servers, devices and macros from the system topology

        This function will return the devices and servers which have been
        removed from the topology in separate lists ``devices`` and ``servers``
        """
        devices, servers = [], []

        if 'device' in system_hash:
            self._topology_device_gone('device', system_hash, devices)
        if 'macro' in system_hash:
            self._topology_device_gone('macro', system_hash, devices)
        if 'server' in system_hash:
            self._topology_server_gone(system_hash, servers)
        if 'client' in system_hash:
            self._topology_client_gone(system_hash)

        return devices, servers

    def _topology_server_gone(self, system_hash, servers):
        """Servers were gone from the systemTopology

        :param servers: List of servers to be extended which are removed
                        from the topology
        """
        for instance_id in system_hash['server'].keys():
            path = 'server' + '.' + instance_id
            attributes = self.get_attributes(path)
            if attributes is None:
                continue

            # Remove instance from system hash
            del self._system_hash[path]

            self.system_tree.remove_server(instance_id)
            for key in self._class_proxies.keys():
                if key[0] == instance_id:
                    # Clear the class proxy binding values when device
                    # server goes offline
                    proxy = self._class_proxies[key]
                    proxy.binding.value.clear_namespace()

            self._class_schemas.setdefault(instance_id, {}).clear()
            # And erase all potential requested class schemas
            self._requested_classes.pop(instance_id, None)
            # Update status of all offline project device proxies
            self._project_device_proxies_server_update(instance_id)

            # Note the details of what device is gone
            host = attributes['host']
            servers.append((instance_id, host, ProxyStatus.OFFLINE))

    def _topology_device_gone(self, instance_type, system_hash, devices):
        """Check if devices or macros are gone in the system topology

        :param devices: List of devices and macros to be extended which are
                        removed from the topology
        """
        for instance_id, _, attr in system_hash[instance_type].iterall():
            # Get the attributes of the instance which is now gone
            path = instance_type + '.' + instance_id
            attributes = self.get_attributes(path)
            if attributes is None:
                continue

            # Remove instance from system hash
            del self._system_hash[path]

            # Set the DeviceProxy to offline, but keep it.
            proxy = self._device_proxies.get(instance_id)
            if proxy is not None:
                proxy.status = ProxyStatus.OFFLINE

            # Update the trees
            self.system_tree.remove_device(instance_id)
            self.device_tree.remove_device(instance_id)

            # Note the details of what device is gone
            class_id = attributes['classId']
            devices.append((instance_id, class_id, ProxyStatus.OFFLINE))

    def _topology_client_gone(self, system_hash):
        for instance_id, _, attr in system_hash['client'].iterall():
            path = 'client' + '.' + instance_id
            attrs = self.get_attributes(path)
            if attrs is None:
                continue
            del self._system_hash[path]

    # ---------------------------------------------------------------------
    # Utilities

    def _request_server_classes(self, server_hash):
        """Topology changed so send new class schema requests"""
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

    def _get_device_attributes(self, device_id):
        """Get the attributes for a device, if it's online. Otherwise get None.
        """
        for parent in ('device', 'macro'):
            attrs = self.get_attributes(parent + '.' + device_id)
            if attrs is not None:
                break
        return attrs

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
