#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import Counter

from karabogui.singletons.api import get_topology


def is_server_online(serverId):
    """ Walk the system topology and return online server status
    """
    topology = get_topology()
    success = False

    def visitor(node):
        nonlocal success
        if (node.attributes.get('type') == 'server'
                and node.node_id == serverId):
            success = True
            return True

    topology.visit_system_tree(visitor)

    return success


def get_macro_servers():
    """ get an ordered list of macro servers

    Walk the system topology and return a list of
    macro servers ordered from least to most loaded
    """
    topology = get_topology()
    # macro_server is a Counter object that will contain
    # one entry per macro server set to the number of macros
    # in that macro server + 1
    macro_servers = Counter()

    def visitor(node):
        nonlocal macro_servers
        attrs = node.attributes
        if (attrs.get('type') == 'server'
                and attrs.get('lang', '') == 'macro'):
            macro_servers[node.node_id] += 1
        elif (attrs.get('type') == 'server'
                and node.node_id == 'karabo/macroServer'):
            # TODO: remove this elif clause after deprecation
            #       karabo/macroServer as hardcoded identifier
            from warnings import warn
            msg = "Hardcoded macroserver definition is deprecated"
            warn(msg, DeprecationWarning)
            macro_servers[node.node_id] += 1
        elif (attrs.get('type') == 'macro'
                and attrs.get('serverId') is not None
                and attrs.get('serverId') != '__none__'):
            macro_servers[attrs['serverId']] += 1

    topology.visit_system_tree(visitor)
    return sorted(macro_servers, key=macro_servers.get)
