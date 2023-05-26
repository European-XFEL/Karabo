# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

from karabo.common.alarm_conditions import AlarmCondition
from karabo.common.states import State
from karathon import DeviceClient as BoundDeviceClient, Hash, LeafType


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
                paramHash = schema.getParameterHash()
                leafType = None
                if paramHash.hasAttribute(key, "leafType"):
                    leafType = paramHash.getAttribute(key, "leafType")
                if leafType == LeafType.STATE:
                    return State(value)
                elif leafType == LeafType.ALARM_CONDITION:
                    return AlarmCondition(value)

        return value
