#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 12, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from contextlib import contextmanager
import re

from traits.api import (HasStrictTraits, Bool, Dict, Enum, Event, Instance,
                        Int, List, on_trait_change, String, WeakRef)

from karabo.common.api import DeviceStatus
from karabo.middlelayer import AccessLevel
from karabo_gui.alarms.api import AlarmInfo
from karabo_gui.enums import NavigationItemTypes
import karabo_gui.globals as krb_globals


class SystemTreeNode(HasStrictTraits):
    """A node in the SystemTree
    """
    node_id = String
    path = String
    visibility = Enum(*AccessLevel)
    status = Enum(*DeviceStatus)
    capabilities = Int
    attributes = Dict
    # Struct to keep track of all alarms related to this
    alarm_info = Instance(AlarmInfo, args=())
    monitoring = Bool(False)

    # Child device counter, always 1 for device node
    device_counter = Int(0)

    parent = WeakRef('SystemTreeNode')
    children = List(Instance('SystemTreeNode'))

    def child(self, node_id):
        for child in self.children:
            if child.node_id == node_id:
                return child
        return None

    def get_children(self, check_counter):
        def visible(node):
            return not (node.visibility > krb_globals.GLOBAL_ACCESS_LEVEL or
                        (check_counter and node.device_counter < 1))
        return [c for c in self.children if visible(c)]

    def info(self):
        level = self.level()
        if level == 0:
            return {'type': NavigationItemTypes.HOST}
        elif level == 1:
            return {'type': NavigationItemTypes.SERVER,
                    'serverId': self.node_id}
        elif level == 2:
            return {'type': NavigationItemTypes.CLASS,
                    'serverId': self.parent.node_id,
                    'classId': self.node_id,
                    'deviceId': ''}
        elif level == 3:
            return {'type': NavigationItemTypes.DEVICE,
                    'classId': self.parent.node_id,
                    'deviceId': self.node_id}

    def level(self):
        level = -1
        temp = self
        while temp.parent is not None:
            level += 1
            temp = temp.parent
        return level

    def row(self):
        if self.parent is None:
            return 0
        return self.parent.children.index(self)

    def append_alarm_type(self, dev_property, alarm_type):
        """Append given ``alarm_type`` to dict and update list with device
        properties
        """
        self.alarm_info.append_alarm_type(dev_property, alarm_type)

    def remove_alarm_type(self, dev_property, alarm_type):
        """Remove given ``dev_property`` with ``alarm_type`` from dict list
        or remove ``alarm_type`` from dict
        """
        self.alarm_info.remove_alarm_type(dev_property, alarm_type)

    @on_trait_change('children.device_counter')
    def _recount(self):
        count = 0
        for child in self.children:
            count += child.device_counter
        self.device_counter = count


class SystemTree(HasStrictTraits):
    """A data model which holds data concerning the devices and servers in a
    running Karabo system.
    """
    # Base node for the whole tree
    root = Instance(SystemTreeNode, args=())

    # A context manager to enter when manipulating the tree
    update_context = Instance(object)
    # An event which is triggered whenever the tree needs to be updated
    needs_update = Event

    def clear_all(self):
        """Removes all data from the model.
        """
        with self.update_context.reset_context():
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
            if not self.find(server_id, full_match=True):
                continue

            server_class_keys.extend(self.remove_server(server_id))
            existing_devices.append(server_id)

        for device_id in device_hash.keys():
            # Check, if device_id is already in tree
            if not self.find(device_id, full_match=True):
                continue

            self.remove_device(device_id)
            existing_devices.append(device_id)

        return existing_devices, server_class_keys

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
        access_level = (krb_globals.GLOBAL_ACCESS_LEVEL if access_level is None
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

    def remove_device(self, instance_id):
        """Remove the entry for a device from the tree
        """
        # XXX: TODO remove dependence on the AccessLevel in the model
        # Use admin level to find all nodes, leave no orphan node behind
        nodes = self.find(instance_id, access_level=AccessLevel.ADMIN,
                          full_match=True)
        for node in nodes:
            with self.update_context.removal_context(node):
                node.parent.children.remove(node)

    def remove_server(self, instance_id):
        """Remove the entry for a server from the tree
        """
        server_nodes = self.find(instance_id, full_match=True)
        server_class_keys = []

        for server_node in server_nodes:
            # Take care of removing all children from bottom to top
            while server_node.children:
                class_node = server_node.children[-1]
                while class_node.children:
                    # Just for security take care of devices
                    device_node = class_node.children[-1]
                    # Remove device node
                    with self.update_context.removal_context(device_node):
                        class_node.children.pop()

                # Remove class node
                with self.update_context.removal_context(class_node):
                    key = (server_node.node_id, class_node.node_id)
                    server_class_keys.append(key)
                    server_node.children.pop()

            # Finally remove server node
            host_node = server_node.parent
            with self.update_context.removal_context(server_node):
                host_node.children.remove(server_node)

        return server_class_keys

    def update(self, system_hash):
        """This function is called whenever the system topology has changed
        and the view needs an update.

        The incoming ``config`` represents the system topology.

        Returns a dictionary of ``SystemTreeNode`` instances for newly added
        device instances.
        """
        self._handle_server_data(system_hash)
        nodes = self._handle_device_data('device', system_hash)
        nodes.update(self._handle_device_data('macro', system_hash))
        self.needs_update = True

        return nodes

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
                host_node = SystemTreeNode(node_id=host, path=host,
                                           parent=self.root)
                self._append_child_node(self.root, host_node)

            # Create node for server
            server_node = host_node.child(server_id)
            if server_node is None:
                server_node = SystemTreeNode(node_id=server_id,
                                             path=server_id, parent=host_node)
                self._append_child_node(host_node, server_node)
            server_node.visibility = visibility
            server_node.attributes = attrs
            for class_id, vis in zip(attrs.get('deviceClasses', []),
                                     attrs.get('visibilities', [])):
                self._handle_class_data(server_node, class_id, vis)

    def _handle_device_data(self, device_type, system_hash):
        """Put the contents of Hash `system_hash` into the internal tree
        structure. Returns a dictionary of any newly added device
        ``SystemTreeNode`` instances.
        """
        new_dev_nodes = {}
        assert device_type in ('device', 'macro')

        if device_type not in system_hash:
            return new_dev_nodes

        for device_id, _, attrs in system_hash[device_type].iterall():
            if len(attrs) == 0:
                continue

            host = attrs.get('host', 'UNKNOWN')
            visibility = AccessLevel(attrs.get('visibility',
                                               AccessLevel.OBSERVER))
            capabilities = attrs.get('capabilities', 0)
            server_id = attrs.get('serverId', 'unknown-server')
            class_id = attrs.get('classId', 'unknown-class')
            status = DeviceStatus(attrs.get('status', 'ok'))

            # Host node
            host_node = self.root.child(host)
            if host_node is None:
                host_node = SystemTreeNode(node_id=host, path=host,
                                           parent=self.root)
                self._append_child_node(self.root, host_node)

            # Server node
            server_node = host_node.child(server_id)
            if server_node is None:
                if server_id == "__none__":
                    server_node = SystemTreeNode(node_id=server_id,
                                                 path=server_id,
                                                 parent=host_node)
                    self._append_child_node(host_node, server_node)
                else:
                    continue

            # Class node
            class_node = server_node.child(class_id)
            if class_node is None:
                if server_id == "__none__" or device_type == 'macro':
                    path = "{}.{}".format(server_id, class_id)
                    class_node = SystemTreeNode(node_id=class_id, path=path,
                                                parent=server_node)
                    self._append_child_node(server_node, class_node)
                else:
                    # XXX: a fix to see running devices - the actual bug lies
                    # in the DeviceClient of the GuiServerDevice which _forgot_
                    # about the attributes "deviceClasses" and "visibilities"
                    class_node = self._handle_class_data(server_node, class_id,
                                                         visibility)

            # Device node
            device_node = class_node.child(device_id)
            if device_node is None:
                device_node = SystemTreeNode(node_id=device_id, path=device_id,
                                             parent=class_node,
                                             device_counter=1)
                self._append_child_node(class_node, device_node)
                device_node.monitoring = False
                # new nodes should be returned
                new_dev_nodes[device_id] = device_node

            device_node.status = status
            device_node.visibility = visibility
            device_node.attributes = attrs
            device_node.capabilities = capabilities

        return new_dev_nodes

    def _handle_class_data(self, server_node, class_id, visibility):
        class_node = server_node.child(class_id)
        if class_node is None:
            server_id = server_node.node_id
            path = '{}.{}'.format(server_id, class_id)
            class_node = SystemTreeNode(node_id=class_id,
                                        path=path, parent=server_node)
            self._append_child_node(server_node, class_node)
        class_node.visibility = AccessLevel(visibility)
        return class_node
