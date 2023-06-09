#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
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
