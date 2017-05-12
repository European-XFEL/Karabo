#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


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
