#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import Enum, Instance, String

from .bases import BaseProjectObjectModel


class DeviceConfigurationModel(BaseProjectObjectModel):
    """ A single device configuration
    """
    configuration = Instance(object)
    server_id = String
    class_id = String
    instance_id = String
    if_exists = Enum('ignore', 'restart')
