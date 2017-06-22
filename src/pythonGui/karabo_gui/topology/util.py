#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from karabo_gui.singletons.api import get_topology


def clear_configuration_instance(configuration, redummy=True):
    """Clear some of the state built up in a Configuration object

    :param configuration: The Configuration object
    :param redummy: Flag states whether we want to `redummy` the Configuration
                    object or let the descriptor stay set
    """
    if configuration is None:
        return

    if configuration.parameterEditor is not None:
        configuration.parameterEditor.clear()
        configuration.parameterEditor = None

    if redummy and configuration.descriptor is not None:
        configuration.redummy()


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
