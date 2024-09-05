#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 18, 2017
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
from collections import Counter
from copy import deepcopy

from karabo.common.api import ServerFlags
from karabo.native import Hash
from karabogui.singletons.api import get_topology


def _is_online(prefix, name):
    topology = get_topology()
    path = prefix + "." + name
    attrs = topology.get_attributes(path)
    return True if attrs else False


def is_server_online(serverId):
    """Check the system topology and return online server status"""
    return _is_online("server", serverId)


def is_device_online(deviceId):
    """Check the system topology and return online device status"""
    return _is_online("device", deviceId)


def is_macro_online(deviceId):
    """Check the system topology and return online macro status"""
    return _is_online("macro", deviceId)


def get_macro_servers(development=False):
    """ get an ordered list of macro servers

    Walk the system topology and return a list of
    macro servers ordered from least to most loaded

    :param development: Denote if development macro servers should be included
                        The default is `False`
    """
    topology = get_topology()
    # macro_server is a Counter object that will contain one entry per macro
    # server set to the number of macros in that macro server + 1
    macro_servers = Counter()
    # macro_servers_set contains all server of `macro` type.
    macro_servers_set = set()

    def visitor(node):
        nonlocal macro_servers
        attrs = node.attributes
        if attrs.get("type") == "server" and attrs.get("lang") == "macro":
            # Exclude development servers
            if (not development and attrs.get("serverFlags", 0)
                    & ServerFlags.Development == ServerFlags.Development):
                return
            macro_servers[attrs["serverId"]] += 1
            macro_servers_set.add(node.node_id)
        elif (attrs.get("type") == "macro"
              and attrs.get("serverId") is not None
              and attrs.get("serverId") != "__none__"):
            macro_servers[attrs["serverId"]] += 1

    topology.visit_system_tree(visitor)

    return [server for server in sorted(macro_servers, key=macro_servers.get)
            if server in macro_servers_set]


def getTopology():
    """Return a deepcopy of the system topology hash"""
    topology = get_topology()._system_hash or Hash()
    return deepcopy(topology)
