#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import Instance, String

from .bases import BaseProjectObjectModel


class DeviceConfigurationModel(BaseProjectObjectModel):
    """ A single device configuration
    """
    class_id = String
    configuration = Instance(object)
