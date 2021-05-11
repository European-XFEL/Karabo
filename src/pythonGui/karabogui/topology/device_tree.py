#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on February 12, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import re
from contextlib import contextmanager

from traits.api import (
    Bool, Dict, Enum, Event, HasStrictTraits, Instance, Int, List, String,
    WeakRef)

import karabogui.access as krb_access
from karabo.common.api import ProxyStatus
from karabo.native import AccessLevel
from karabogui.itemtypes import NavigationItemTypes

BLACKLIST_CLASSES = ('DataLogger', 'DataLogReader', 'GuiServerDevice',
                     'LogAggregator', 'ProjectManager')
DOMAIN_LEVEL = 0
TYPE_LEVEL = 1
MEMBER_LEVEL = 2


class DeviceTreeNode(HasStrictTraits):
    """A node in the DeviceTree
    """
    node_id = String
    server_id = String
    visibility = Enum(*AccessLevel)
    status = Enum(*ProxyStatus)
    capabilities = Int
    interfaces = Int
    attributes = Dict
    level = Int(-1)

    parent = WeakRef('DeviceTreeNode')
    children = List(Instance('DeviceTreeNode'))

    # cached current visibility
    is_visible = Bool(True)

    def _is_visible_default(self):
        return not (self.visibility > krb_access.GLOBAL_ACCESS_LEVEL)

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
        node = self._device_nodes.get(instance_id)
        if node is None:
            return False

        self._device_nodes.pop(instance_id)

        type_node = node.parent
        with self.update_context.removal_context(node):
            type_node.children.remove(node)

        if not len(type_node.children):
            domain_node = type_node.parent
            with self.update_context.removal_context(type_node):
                domain_node.children.remove(type_node)

        return True

    def initialize(self, system_hash):
        with self.update_context.reset_context():
            self._handle_device_data('device', system_hash, append=False)

    def update(self, system_hash):
        with self.update_context.layout_context():
            nodes = self._handle_device_data('device', system_hash)

        return nodes

    def instance_update(self, system_hash):
        if "device" not in system_hash:
            return

        device_status = set()
        for device_id, _, attrs in system_hash['device'].iterall():
            device_node = self._device_nodes.get(device_id)
            if device_node is None:
                continue

            status = ProxyStatus(attrs.get('status', 'ok'))
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

        class Dummy(object):
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

        return Dummy()

    def _append_child_node(self, parent_node, child_node):
        # First and last indices, INCLUSIVE
        first = len(parent_node.children)
        last = first

        with self.update_context.insertion_context(parent_node, first, last):
            parent_node.children.append(child_node)

    def _set_child_node(self, parent_node, child_node):
        parent_node.children.append(child_node)

    # ------------------------------------------------------------------

    def _handle_device_data(self, device_type, system_hash, append=True):
        assert device_type in ('device', 'macro')
        handler = self._append_child_node if append else self._set_child_node

        if device_type not in system_hash:
            return

        for karabo_name, _, attrs in system_hash[device_type].iterall():
            if attrs.get('classId', '') in BLACKLIST_CLASSES:
                continue

            # If we don't follow karabo naming convention, we are out!
            device = karabo_name.split('/')
            if not len(device) == 3:
                continue

            domain, dev_type, member = device
            visibility = AccessLevel(attrs.get('visibility',
                                               AccessLevel.OBSERVER))
            capabilities = attrs.get('capabilities', 0)
            interfaces = attrs.get('interfaces', 0)
            server_id = attrs.get('serverId', 'unknown-server')
            status = ProxyStatus(attrs.get('status', 'ok'))

            domain_node = self.root.child(domain)
            if domain_node is None:
                domain_node = DeviceTreeNode(node_id=domain,
                                             parent=self.root,
                                             level=DOMAIN_LEVEL)
                handler(self.root, domain_node)

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
                                             visibility=visibility,
                                             level=MEMBER_LEVEL)
                handler(type_node, device_node)

            device_node.status = status
            device_node.attributes = attrs
            device_node.capabilities = capabilities
            device_node.interfaces = interfaces

            self._device_nodes[karabo_name] = device_node

    def find(self, node_id, access_level=None, case_sensitive=True,
             use_reg_ex=True, full_match=False):
        """Find all nodes with the given `node_id` and return them in a list

        :param node_id: The actual string we are looking for in the tree
        :param access_level: The global access level
        :param case_sensitive: States whether the given string should match
                               case or not
        :param full_match: States whether the given string matches fully
        :param use_reg_ex: Defines the given string as a regular expression
        :return list of found nodes (which could be empty)
        """
        access_level = (
            krb_access.GLOBAL_ACCESS_LEVEL if access_level is None
            else access_level)

        found_nodes = []
        pattern = node_id if use_reg_ex else ".*{}".format(re.escape(node_id))
        flags = 0 if case_sensitive else re.IGNORECASE
        regex = re.compile(pattern, flags=flags)

        def visitor(node):
            parent_node = node.parent
            parent_check = (parent_node is not None
                            and parent_node.visibility > access_level)
            if (node.visibility > access_level or parent_check):
                # Do not look for nodes which are not visible or its parent
                return

            matcher = regex.fullmatch if full_match else regex.match
            if matcher(node.node_id) is not None:
                found_nodes.append(node)

        self.visit(visitor)

        return found_nodes
