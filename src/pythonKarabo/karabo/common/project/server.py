#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import Instance, List

from .bases import BaseProjectObjectModel
from .device import DeviceConfigurationModel


class DeviceServerModel(BaseProjectObjectModel):
    """ An object representing a device server
    """
    devices = List(Instance(DeviceConfigurationModel))
