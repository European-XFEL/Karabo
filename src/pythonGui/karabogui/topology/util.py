#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
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

    topology.visit_system_tree(visitor)

    return success
