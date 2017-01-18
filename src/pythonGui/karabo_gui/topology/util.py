#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on January 18, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


def clear_configuration_instance(configuration):
    """Clear some of the state built up in a Configuration object.
    """
    if configuration is None:
        return

    if configuration.parameterEditor is not None:
        configuration.parameterEditor.clear()

    if configuration.descriptor is not None:
        configuration.redummy()
