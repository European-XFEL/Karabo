#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on February 12, 2019
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
    Dict, Enum, Event, HasStrictTraits, Instance, Int, List, String, WeakRef)

from karabo.common.api import InstanceStatus
from karabogui.itemtypes import NavigationItemTypes

DOMAIN_LEVEL = 0
TYPE_LEVEL = 1
MEMBER_LEVEL = 2


class DeviceTreeNode(HasStrictTraits):
    """A node in the DeviceTree
    """
    node_id = String
    server_id = String
    status = Enum(*InstanceStatus)
    capabilities = Int
    interfaces = Int
    attributes = Dict
    level = Int(-1)

    parent = WeakRef('DeviceTreeNode')
    children = List(Instance('DeviceTreeNode'))

    def child(self, node_id):
        for child in self.children:
            if child.node_id == node_id:
                return child
        return None

    def info(self):
        level = self.level
        if level == DOMAIN_LEVEL:
            return {'type': NavigationItemTypes.UNDEFINED,
                    'domainId': self.node_id}
        elif level == TYPE_LEVEL:
            return {'type': NavigationItemTypes.UNDEFINED,
                    'typeId': self.node_id}
        elif level == MEMBER_LEVEL:
            return {'type': NavigationItemTypes.DEVICE,
                    'deviceId': self.node_id,
                    'serverId': self.server_id,
                    'capabilities': self.capabilities,
                    'archive': self.attributes.get('archive', False)}

    def row(self):
        if self.parent is None:
            return 0
        return self.parent.children.index(self)


class DeviceSystemTree(HasStrictTraits):
    # Base node for the whole tree
    root = Instance(DeviceTreeNode, args=())

    # An event which is triggered the device statuses where updated
    status_update = Event

    # A context manager to enter when manipulating the tree
    update_context = Instance(object)

    # device/server node lookup dicts
    _device_nodes = Dict

    def clear_all(self):
        """Removes all data from the model.
        """
        self._device_nodes.clear()

        with self.update_context.reset_context():
            self.root.children = []

    def get_instance_node(self, instance_id):
        """Retrieve a tree node for a device instance"""
        return self._device_nodes.get(instance_id, None)

    def remove_device(self, instance_id):
        """Remove the entry for a device from the tree
        """
        node = self._device_nodes.pop(instance_id, None)
        if node is None:
            return False

        type_node = node.parent
        with self.update_context.removal_context(node):
            type_node.children.remove(node)

        if not type_node.children:
            domain_node = type_node.parent
            with self.update_context.removal_context(type_node):
                domain_node.children.remove(type_node)

            if not domain_node.children:
                self._remove_root_children(domain_node)

        return True

    def initialize(self, system_hash):
        with self.update_context.reset_context():
            self._handle_device_data('device', system_hash, append=False)

    def update(self, system_hash):
        with self.update_context.layout_context():
            self._handle_device_data('device', system_hash)

    def instance_update(self, system_hash):
        if "device" not in system_hash:
            return

        device_status = set()
        for device_id, _, attrs in system_hash['device'].iterall():
            device_node = self._device_nodes.get(device_id)
            if device_node is None:
                continue

            status = InstanceStatus(attrs['status'])
            device_node.status = status
            device_node.attributes = attrs
            device_status.add(device_id)

        if device_status:
            self.status_update = device_status

    def visit(self, visitor):
        """Walk each node of the device tree until visitor returns true

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

        with self.update_context.insertion_context(parent_node, first, last):
            parent_node.children.append(child_node)

    def _set_child_node(self, parent_node, child_node):
        parent_node.children.append(child_node)

    def _set_root_children(self, node, append):
        """Set the a domain node in the root element"""
        if append:
            first = len(self.root.children)
            last = first
            with self.update_context.insert_root_context(first, last):
                self.root.children.append(node)
        else:
            self.root.children.append(node)

    def _remove_root_children(self, node):
        """Set a node in the root element"""
        first = node.row()
        last = first
        with self.update_context.remove_root_context(first, last):
            self.root.children.remove(node)

    # ------------------------------------------------------------------

    def _handle_device_data(self, device_type, system_hash, append=True):
        assert device_type in ('device', 'macro')
        handler = self._append_child_node if append else self._set_child_node

        if device_type not in system_hash:
            return

        for karabo_name, _, attrs in system_hash[device_type].iterall():
            if len(attrs) == 0:
                continue
            # If we don't follow karabo naming convention, we are out!
            device = karabo_name.split('/')
            if not len(device) == 3:
                continue

            domain, dev_type, member = device
            capabilities = attrs['capabilities']
            server_id = attrs['serverId']
            status = InstanceStatus(attrs['status'])
            # Interfaces are optional ...
            interfaces = attrs.get('interfaces', 0)

            domain_node = self.root.child(domain)
            if domain_node is None:
                domain_node = DeviceTreeNode(node_id=domain,
                                             parent=self.root,
                                             level=DOMAIN_LEVEL)
                self._set_root_children(domain_node, append)

            type_node = domain_node.child(dev_type)
            if type_node is None:
                type_node = DeviceTreeNode(node_id=dev_type,
                                           parent=domain_node,
                                           level=TYPE_LEVEL)
                handler(domain_node, type_node)

            device_node = type_node.child(karabo_name)
            if device_node is None:
                device_node = DeviceTreeNode(node_id=karabo_name,
                                             parent=type_node,
                                             server_id=server_id,
                                             status=status,
                                             attributes=attrs,
                                             capabilities=capabilities,
                                             interfaces=interfaces,
                                             level=MEMBER_LEVEL)
                handler(type_node, device_node)

            self._device_nodes[karabo_name] = device_node

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
