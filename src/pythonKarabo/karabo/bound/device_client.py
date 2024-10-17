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

from karabind import DeviceClient as BoundDeviceClient, Hash
from karabo.common.alarm_conditions import AlarmCondition
from karabo.common.api import KARABO_CLASS_ID_ALARM, KARABO_CLASS_ID_STATE
from karabo.common.states import State


class DeviceClient(BoundDeviceClient):
    """ Handle certain device client responsibilities which interact with other
    Python code in Karabo.
    """

    def get(self, instanceId, *args, **kw):
        """ Handle conversion of returned State and AlarmCondition objects."""
        value = super().get(instanceId, *args, **kw)

        if not isinstance(value, Hash):
            schema = self.getDeviceSchema(instanceId)
            key = args[0]
            if schema.isLeaf(key):
                if schema.hasClassId(key):
                    classId = schema.getClassId(key)
                    if classId == KARABO_CLASS_ID_STATE:
                        return State(value)
                    elif classId == KARABO_CLASS_ID_ALARM:
                        return AlarmCondition(value)

        return value
