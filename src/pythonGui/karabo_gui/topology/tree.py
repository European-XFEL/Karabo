#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 12, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from traits.api import (HasStrictTraits, Any, Bool, Dict, Enum, Instance, List,
                        String, WeakRef)

from karabo.middlelayer import AccessLevel
from karabo_gui.enums import NavigationItemTypes


class SystemTreeNode(HasStrictTraits):
    """A node in the SystemTree
    """
    display_name = String
    path = String
    visibility = Enum(*list(AccessLevel))
    status = String
    attributes = Dict
    alarm_type = Any
    monitoring = Bool(False)

    parent = WeakRef('SystemTreeNode')
    children = List(Instance('SystemTreeNode'))

    def child(self, display_name):
        for child in self.children:
            if child.display_name == display_name:
                return child
        return None

    def info(self):
        level = self.level()
        if level == 0:
            return {'type': NavigationItemTypes.HOST}
        elif level == 1:
            return {'type': NavigationItemTypes.SERVER,
                    'serverId': self.display_name}
        elif level == 2:
            return {'type': NavigationItemTypes.CLASS,
                    'serverId': self.parent.display_name,
                    'classId': self.display_name,
                    'deviceId': ''}
        elif level == 3:
            return {'type': NavigationItemTypes.DEVICE,
                    'classId': self.parent.display_name,
                    'deviceId': self.display_name}

    def level(self):
        level = -1
        temp = self
        while temp.parent is not None:
            level += 1
            temp = temp.parent
        return level

    def row(self):
        if not self.parent:
            return None
        return self.parent.children.index(self)


class SystemTree(HasStrictTraits):
    """A data model which holds data concerning the devices and servers in a
    running Karabo system.
    """
    # Base node for the whole tree
    root = Instance(SystemTreeNode, args=())

    # A context manager to enter when manipulating the tree
    update_context = Instance(object)

    def clear_all(self):
        """Removes all data from the model.
        """
        with self.update_context:
            self.root.children = []

    def clear_existing(self, system_hash):
        """Checks whether instances already exist in the tree and removes them.

        Returns a list with all existing device_ids and a list with existing
        server class keys (server_id, class_id).
        """
        existing_devices, server_class_keys = [], []
        server_hash = system_hash.get('server', {})
        device_hash = system_hash.get('device', {})

        for server_id in server_hash.keys():
            # Check, if server_id is already in tree
            if self.find(server_id) is None:
                continue

            server_class_keys.extend(self.remove_server(server_id))
            existing_devices.append(server_id)

        for device_id in device_hash.keys():
            # Check, if device_id is already in tree
            if self.find(device_id) is None:
                continue

            self.remove_device(device_id)
            existing_devices.append(device_id)

        return existing_devices, server_class_keys

    def find(self, node_path):
        """Find a node by its path
        """
        def recurse(node, path):
            for i in range(len(node.children)):
                child = node.children[i]
                result = recurse(child, path)
                if result is not None:
                    return result

            if node.path != "" and path == node.path:
                return node

            return None

        return recurse(self.root, node_path)

    def remove_device(self, instance_id):
        """Remove the entry for a device from the tree
        """
        node = self.find(instance_id)
        if node is not None:
            with self.update_context:
                node.parent.children.remove(node)

    def remove_server(self, instance_id):
        """Remove the entry for a server from the tree
        """
        node = self.find(instance_id)
        if node is None:
            return []

        server_class_keys = []
        with self.update_context:
            parent = node.parent
            # There are children, if server
            for child in node.children:
                key = (node.display_name, child.display_name)
                server_class_keys.append(key)
            parent.children.remove(node)

        return server_class_keys

    def update(self, system_hash):
        """This function is called whenever the system topology has changed
        and the view needs an update.

        The incoming ``config`` represents the system topology.
        """
        with self.update_context:
            self._handle_server_data(system_hash)
            self._handle_device_data('device', system_hash)
            self._handle_device_data('macro', system_hash)

    # ------------------------------------------------------------------

    def _update_context_default(self):
        """Traits default initializer for the `update_context` trait.
        """
        class Dummy(object):
            def __enter__(self):
                return self

            def __exit__(self, *exc):
                return False

        return Dummy()

    # ------------------------------------------------------------------

    def _handle_server_data(self, system_hash):
        """Put the contents of Hash `system_hash` into the internal tree
        structure.
        """
        if 'server' not in system_hash:
            return

        for server_id, _, attrs in system_hash['server'].iterall():
            if len(attrs) == 0:
                continue

            host = attrs.get('host', 'UNKNOWN')
            visibility = AccessLevel(attrs.get('visibility',
                                               AccessLevel.OBSERVER))

            # Create node for host
            host_node = self.root.child(host)
            if host_node is None:
                host_node = SystemTreeNode(display_name=host, path=host,
                                           parent=self.root)
                self.root.children.append(host_node)

            # Create node for server
            server_node = host_node.child(server_id)
            if server_node is None:
                server_node = SystemTreeNode(display_name=server_id,
                                             path=server_id, parent=host_node)
                host_node.children.append(server_node)
            server_node.visibility = visibility
            server_node.attributes = attrs

            for class_id, vis in zip(attrs.get('deviceClasses', []),
                                     attrs.get('visibilities', [])):
                path = '{}.{}'.format(server_id, class_id)
                class_node = server_node.child(class_id)
                if class_node is None:
                    class_node = SystemTreeNode(display_name=class_id,
                                                path=path, parent=server_node)
                    server_node.children.append(class_node)
                class_node.visibility = AccessLevel(vis)

    def _handle_device_data(self, device_type, system_hash):
        """Put the contents of Hash `system_hash` into the internal tree
        structure.
        """
        assert device_type in ('device', 'macro')

        if device_type not in system_hash:
            return

        for device_id, _, attrs in system_hash[device_type].iterall():
            if len(attrs) == 0:
                continue

            host = attrs.get('host', 'UNKNOWN')
            visibility = AccessLevel(attrs.get('visibility',
                                               AccessLevel.OBSERVER))
            server_id = attrs.get('serverId', 'unknown-server')
            class_id = attrs.get('classId', 'unknown-class')
            status = attrs.get('status', 'ok')

            # Host node
            host_node = self.root.child(host)
            if host_node is None:
                host_node = SystemTreeNode(display_name=host, path=host,
                                           parent=self.root)
                self.root.children.append(host_node)

            # Server node
            server_node = host_node.child(server_id)
            if server_node is None:
                if server_id == "__none__":
                    server_node = SystemTreeNode(display_name=server_id,
                                                 path=server_id,
                                                 parent=host_node)
                    host_node.children.append(server_node)
                else:
                    continue

            # Class node
            class_node = server_node.child(class_id)
            if class_node is None:
                if server_id == "__none__" or device_type == 'macro':
                    path = "{}.{}".format(server_id, class_id)
                    class_node = SystemTreeNode(display_name=class_id,
                                                path=path,
                                                parent=server_node)
                    server_node.children.append(class_node)
                else:
                    continue

            # Device node
            device_node = class_node.child(device_id)
            if device_node is None:
                device_node = SystemTreeNode(display_name=device_id,
                                             path=device_id, parent=class_node)
                class_node.children.append(device_node)
                device_node.monitoring = False
            device_node.status = status
            device_node.visibility = visibility
            device_node.attributes = attrs
