#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 12, 2017
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
import re
from contextlib import contextmanager

from traits.api import (
    Bool, Dict, Enum, Event, HasStrictTraits, Instance, Int, List, String,
    WeakRef)

from karabo.common.api import InstanceStatus
from karabogui.itemtypes import NavigationItemTypes

HOST_LEVEL = 0
SERVER_LEVEL = 1
CLASS_LEVEL = 2
DEVICE_LEVEL = 3


class SystemTreeNode(HasStrictTraits):
    """A node in the SystemTree
    """
    node_id = String
    path = String
    status = Enum(*InstanceStatus)
    capabilities = Int
    attributes = Dict
    # Struct to keep track of all alarms related to this
    monitoring = Bool(False)
    level = Int(-1)

    parent = WeakRef('SystemTreeNode')
    children = List(Instance('SystemTreeNode'))

    def child(self, node_id):
        for child in self.children:
            if child.node_id == node_id:
                return child
        return None

    def info(self):
        level = self.level
        if level == HOST_LEVEL:
            return {'type': NavigationItemTypes.HOST,
                    "hostId": self.node_id}
        elif level == SERVER_LEVEL:
            return {'type': NavigationItemTypes.SERVER,
                    'serverId': self.node_id,
                    'log': self.attributes.get("log", "INFO"),
                    'karaboVersion': self.attributes.get('karaboVersion',
                                                         "0.0.0")}
        elif level == CLASS_LEVEL:
            return {'type': NavigationItemTypes.CLASS,
                    'serverId': self.parent.node_id,
                    'classId': self.node_id,
                    'deviceId': ''}
        elif level == DEVICE_LEVEL:
            return {'type': NavigationItemTypes.DEVICE,
                    'classId': self.parent.node_id,
                    'deviceId': self.node_id,
                    'archive': self.attributes.get("archive", False),
                    'capabilities': self.capabilities}

    def row(self):
        if self.parent is None:
            return 0
        return self.parent.children.index(self)


class SystemTree(HasStrictTraits):
    """A data model which holds data concerning the devices and servers in a
    running Karabo system.
    """
    # Base node for the whole tree
    root = Instance(SystemTreeNode, args=())

    # A context manager to enter when manipulating the tree
    update_context = Instance(object)

    # An event which is triggered whenenver the tree needs a layout update
    status_update = Event

    # device/server node lookup dicts
    _device_nodes = Dict
    _server_nodes = Dict

    # initialized hook
    initialized = Event

    def clear_all(self):
        """Removes all data from the model.
        """
        self._device_nodes.clear()
        self._server_nodes.clear()

        with self.update_context.reset_context():
            self.root.children = []

    def clear_existing(self, system_hash):
        """Checks whether instances already exist in the tree and removes them.

        Returns a list with all existing device_ids and a list with existing
        server class keys (server_id, class_id).
        """
        existing_devices = []
        server_hash = system_hash.get('server', {})
        device_hash = system_hash.get('device', {})

        for server_id in server_hash.keys():
            server_classes = self.remove_server(server_id)
            # Check, if server_id was in tree
            if server_classes:
                existing_devices.append(server_id)

        for device_id in device_hash.keys():
            # device_id might not be in the tree
            if not self.remove_device(device_id):
                continue
            existing_devices.append(device_id)

        return existing_devices

    def find(self, node_id, case_sensitive=True,
             use_reg_ex=True, full_match=False):
        """Find all nodes with the given `node_id` and return them in a list

        :param node_id: The actual string we are looking for in the tree
        :param case_sensitive: States whether the given string should match
                               case or not
        :param full_match: States whether the given string matches fully
        :param use_reg_ex: Defines the given string as a regular expression
        :return list of found nodes (which could be empty)
        """

        found_nodes = []
        pattern = node_id if use_reg_ex else f".*{re.escape(node_id)}"
        flags = 0 if case_sensitive else re.IGNORECASE
        regex = re.compile(pattern, flags=flags)

        def visitor(node):
            parent_node = node.parent
            parent_check = parent_node is None
            if parent_check:
                # Do not look for nodes which are its parent
                return

            matcher = regex.fullmatch if full_match else regex.match
            if matcher(node.node_id) is not None:
                found_nodes.append(node)

        self.visit(visitor)
        return found_nodes

    def get_instance_node(self, instance_id):
        """Retrieve a tree node for a device or server instance"""
        if instance_id in self._device_nodes:
            return self._device_nodes[instance_id]
        if instance_id in self._server_nodes:
            return self._server_nodes[instance_id]
        return None

    def remove_device(self, instance_id):
        """Remove the entry for a device from the tree
        """
        node = self._device_nodes.pop(instance_id, None)
        if node is None:
            return False

        class_node = node.parent
        with self.update_context.removal_context(node):
            class_node.children.remove(node)

        if not class_node.children:
            server_node = class_node.parent
            with self.update_context.removal_context(class_node):
                server_node.children.remove(class_node)

        return True

    def remove_server(self, instance_id):
        """Remove the entry for a server from the tree
        """
        server_node = self._server_nodes.pop(instance_id, None)
        server_class_keys = []

        if server_node is None:
            return server_class_keys

        # Take care of removing all children from bottom to top
        for class_node in server_node.children:
            for device_node in class_node.children:
                self._device_nodes.pop(device_node.node_id, None)
            with self.update_context.removal_children_context(class_node):
                class_node.children = []

        with self.update_context.removal_children_context(server_node):
            # Remove class nodes
            server_node.children = []

        # Finally remove server node
        host_node = server_node.parent
        with self.update_context.removal_context(server_node):
            host_node.children.remove(server_node)

        if not host_node.children:
            self._remove_root_children(host_node)

        return server_class_keys

    def update(self, system_hash):
        """This function is called whenever the system topology has changed
        and the view needs an update.

        The incoming ``config`` represents the system topology.

        Returns a dictionary of ``SystemTreeNode`` instances for newly added
        device instances.
        """
        with self.update_context.layout_context():
            self._handle_server_data(system_hash)
            nodes = self._handle_device_data('device', system_hash)
            nodes.update(self._handle_device_data('macro', system_hash))

        return nodes

    def instance_update(self, system_hash):
        """Put the contents of Hash `system_hash` into the internal tree
        structure.
        """

        device_status = set()

        if 'server' in system_hash:
            for server_id, _, attrs in system_hash['server'].iterall():
                if len(attrs) == 0:
                    continue
                server_node = self._server_nodes.get(server_id, None)
                if server_node is None:
                    continue
                server_node.attributes = attrs

        for device_type in ('device', 'macro'):
            if device_type in system_hash:
                for device_id, _, attrs in system_hash[device_type].iterall():
                    if len(attrs) == 0:
                        continue

                    device_node = self._device_nodes.get(device_id, None)
                    if device_node is None:
                        continue

                    device_status.add(device_id)
                    status = InstanceStatus(attrs['status'])
                    device_node.status = status
                    device_node.attributes = attrs

        if device_status:
            self.status_update = device_status

    def initialize(self, system_hash):
        """Initialize the system topology with a reset context
        """
        with self.update_context.reset_context():
            self._handle_server_data(system_hash, append=False)
            self._handle_device_data('device', system_hash, append=False)
            self._handle_device_data('macro', system_hash, append=False)
        self.initialized = True

    def visit(self, visitor):
        """Walk every node of the system tree until visitor returns true

        Call the `visitor` function on each item, if the function returns
        a true-ish value, the loop will be stopped.
        """

        def _iter_tree_node(node):
            yield node
            for child in node.children:
                yield from _iter_tree_node(child)

        for t_node in _iter_tree_node(self.root):
            if visitor(t_node):
                break

    # ------------------------------------------------------------------

    def _update_context_default(self):
        """Traits default initializer for the `update_context` trait.
        """

        class Dummy:
            @contextmanager
            def reset_context(self):
                yield

            @contextmanager
            def insertion_context(self, parent_node, first, last):
                yield

            @contextmanager
            def removal_context(self, tree_node):
                yield

            @contextmanager
            def layout_context(self):
                yield

            @contextmanager
            def insert_root_context(self, first, last):
                yield

            @contextmanager
            def remove_root_context(self, first, last):
                yield

            @contextmanager
            def removal_children_context(self, tree_node):
                yield

        return Dummy()

    def _append_child_node(self, parent_node, child_node):
        # First and last indices, INCLUSIVE
        first = len(parent_node.children)
        last = first

        if child_node.level == SERVER_LEVEL:
            self._server_nodes[child_node.node_id] = child_node
        elif child_node.level == DEVICE_LEVEL:
            self._device_nodes[child_node.node_id] = child_node

        with self.update_context.insertion_context(parent_node, first, last):
            parent_node.children.append(child_node)

    def _set_child_node(self, parent_node, child_node):
        """Set the child node directly without announcing layout change

        NOTE: This function can be only used in a reset context!
        """
        if child_node.level == SERVER_LEVEL:
            self._server_nodes[child_node.node_id] = child_node
        elif child_node.level == DEVICE_LEVEL:
            self._device_nodes[child_node.node_id] = child_node

        parent_node.children.append(child_node)

    def _set_root_children(self, host_node, append=True):
        """Set the a host node in the root element"""
        if append:
            first = len(self.root.children)
            last = first
            with self.update_context.insert_root_context(first, last):
                self.root.children.append(host_node)
        else:
            self.root.children.append(host_node)

    def _remove_root_children(self, node):
        """Remove a node in the root element"""
        first = node.row()
        last = first
        with self.update_context.remove_root_context(first, last):
            self.root.children.remove(node)

    # ------------------------------------------------------------------

    def _handle_server_data(self, system_hash, append=True):
        """Put the contents of Hash `system_hash` into the internal tree
        structure.
        """
        if 'server' not in system_hash:
            return

        handler = self._append_child_node if append else self._set_child_node
        for server_id, _, attrs in system_hash['server'].iterall():
            if len(attrs) == 0:
                continue

            host = attrs['host']

            # Create node for host
            host_node = self.root.child(host)
            if host_node is None:
                host_node = SystemTreeNode(node_id=host, path=host,
                                           parent=self.root,
                                           level=HOST_LEVEL)
                self._set_root_children(host_node, append)

            # Create node for server
            server_node = host_node.child(server_id)
            if server_node is None:
                server_node = SystemTreeNode(node_id=server_id,
                                             path=server_id, parent=host_node,
                                             level=SERVER_LEVEL)
                handler(host_node, server_node)
            server_node.attributes = attrs

    def _handle_device_data(self, device_type, system_hash, append=True):
        """Put the contents of Hash `system_hash` into the internal tree
        structure. Returns a dictionary of any newly added device
        ``SystemTreeNode`` instances.
        """
        new_dev_nodes = {}
        assert device_type in ('device', 'macro')

        if device_type not in system_hash:
            return new_dev_nodes

        handler = self._append_child_node if append else self._set_child_node
        for device_id, _, attrs in system_hash[device_type].iterall():
            if len(attrs) == 0:
                continue

            host = attrs['host']
            capabilities = attrs['capabilities']
            server_id = attrs['serverId']
            class_id = attrs['classId']
            status = InstanceStatus(attrs['status'])

            # A device must have a server added before with host!
            host_node = self.root.child(host)
            if host_node is None:
                continue

            server_node = host_node.child(server_id)
            if server_node is None:
                continue

            # Class node
            class_node = server_node.child(class_id)
            if class_node is None:
                server_id = server_node.node_id
                path = f'{server_id}.{class_id}'
                class_node = SystemTreeNode(node_id=class_id,
                                            path=path, parent=server_node,
                                            level=CLASS_LEVEL)
                handler(server_node, class_node)

            # Device node
            device_node = class_node.child(device_id)
            if device_node is None:
                device_node = SystemTreeNode(node_id=device_id, path=device_id,
                                             parent=class_node,
                                             status=status,
                                             capabilities=capabilities,
                                             attributes=attrs,
                                             level=DEVICE_LEVEL)
                handler(class_node, device_node)
                # new nodes should be returned
                new_dev_nodes[device_id] = device_node

        return new_dev_nodes
