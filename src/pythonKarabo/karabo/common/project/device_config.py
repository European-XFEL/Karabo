#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from traits.api import Instance, String

from .bases import BaseProjectObjectModel


class DeviceConfigurationModel(BaseProjectObjectModel):
    """A single device configuration"""

    # The Class ID of the device
    class_id = String
    # This is the configuration Hash. ``object`` used here to avoid importing.
    configuration = Instance(object)

    def _simple_name_default(self):
        """Traits default initializer for `simple_name`"""
        return "default"
