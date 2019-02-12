#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on February 12, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from contextlib import contextmanager

from traits.api import (HasStrictTraits, Bool, Dict, Enum, Event, Instance,
                        Int, List, on_trait_change, String, WeakRef)

import karabogui.globals as krb_globals
from karabo.common.api import DeviceStatus
from karabo.native import AccessLevel
from karabogui.enums import NavigationItemTypes

DOMAIN_LEVEL = 0
TYPE_LEVEL = 1
MEMBER_LEVEL = 2


class DeviceTreeNode(HasStrictTraits):
    """A node in the DeviceTree
    """
    device_id = String
    node_id = String
    server_id = String
    visibility = Enum(*AccessLevel)
    status = Enum(*DeviceStatus)
    capabilities = Int
    attributes = Dict
    level = Int(-1)

    parent = WeakRef('DeviceTreeNode')
    children = List(Instance('DeviceTreeNode'))
    _visible_children = List(Instance('DeviceTreeNode'))
    clear_cache = Bool(True)

    # cached current visibility
    is_visible = Bool(True)

    def _is_visible_default(self):
        return not (self.visibility > krb_globals.GLOBAL_ACCESS_LEVEL)

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
                    'memberId': self.node_id,
                    'serverId': self.server_id,
                    'deviceId': self.device_id,
                    'capabilities': self.capabilities}

    def row(self):
        if self.parent is None:
            return 0
        return self.parent.children.index(self)

    @on_trait_change('children[]')
    def _register_clear_cache(self):
        self.clear_cache = True

    def get_visible_children(self):
        if self.clear_cache:
            self._visible_children = [c for c in self.children if c.is_visible]
            self.clear_cache = False
        return self._visible_children


class DeviceSystemTree(HasStrictTraits):
    # Base node for the whole tree
    root = Instance(DeviceTreeNode, args=())

    # A context manager to enter when manipulating the tree
    update_context = Instance(object)
    # An event which is triggered whenever the tree needs to be updated
    needs_update = Event

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

        type_node = node.parent
        with self.update_context.removal_context(node):
            type_node.children.remove(node)

        if not len(type_node.children):
            domain_node = type_node.parent
            with self.update_context.removal_context(type_node):
                domain_node.children.remove(type_node)

            if not len(domain_node.children):
                with self.update_context.removal_context(domain_node):
                    self.root.children.remove(domain_node)

        self._device_nodes.pop(instance_id)
        return True

    def update(self, system_hash):
        nodes = self._handle_device_data('device', system_hash)
        self.needs_update = True

        return nodes

    def visit(self, visitor):
        """Walk every node in the system tree and run a `visitor` function on
        each item.
        """

        def _iter_tree_node(node):
            yield node
            for child in node.children:
                yield from _iter_tree_node(child)

        for t_node in _iter_tree_node(self.root):
            visitor(t_node)

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

        return Dummy()

    def _append_child_node(self, parent_node, child_node):
        # First and last indices, INCLUSIVE
        first = len(parent_node.children)
        last = first

        with self.update_context.insertion_context(parent_node, first, last):
            parent_node.children.append(child_node)

    # ------------------------------------------------------------------

    def _handle_device_data(self, device_type, system_hash):
        new_dev_nodes = {}
        assert device_type in ('device', 'macro')

        if device_type not in system_hash:
            return new_dev_nodes

        for karabo_name, _, attrs in system_hash[device_type].iterall():
            # If we don't follow karabo naming convention, we are out!
            device = karabo_name.split('/')
            if not len(device) == 3:
                continue

            # We rule out dataloggers!
            domain, dev_type, member = device
            if domain.startswith("DataLogger-"):
                continue

            visibility = AccessLevel(attrs.get('visibility',
                                               AccessLevel.OBSERVER))
            capabilities = attrs.get('capabilities', 0)
            server_id = attrs.get('serverId', 'unknown-server')
            status = DeviceStatus(attrs.get('status', 'ok'))

            domain_node = self.root.child(domain)
            if domain_node is None:
                domain_node = DeviceTreeNode(node_id=domain,
                                             parent=self.root,
                                             visibility=visibility,
                                             level=DOMAIN_LEVEL)
                self._append_child_node(self.root, domain_node)

            type_node = domain_node.child(dev_type)
            if type_node is None:
                type_node = DeviceTreeNode(node_id=dev_type,
                                           parent=domain_node,
                                           visibility=visibility,
                                           level=TYPE_LEVEL)
                self._append_child_node(domain_node, type_node)

            member_node = type_node.child(member)
            if member_node is None:
                member_node = DeviceTreeNode(node_id=member,
                                             device_id=karabo_name,
                                             parent=type_node,
                                             server_id=server_id,
                                             visibility=visibility,
                                             level=MEMBER_LEVEL)
                self._append_child_node(type_node, member_node)

            member_node.status = status
            member_node.attributes = attrs
            member_node.capabilities = capabilities

            self._device_nodes[karabo_name] = member_node

        return new_dev_nodes
