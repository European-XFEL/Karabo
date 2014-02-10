#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 23, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from karabo.karathon import AccessLevel

# TODO Karabo will support an global access level and an excpetion list which is deviceId specific
# This requires a function like in SignalSlotable: getAccessLevel(deviceId) in the end


GLOBAL_ACCESS_LEVEL = AccessLevel.OBSERVER
#KARABO_DEFAULT_ACCESS_LEVEL = AccessLevel.OBSERVER  # Inside XFEL
KARABO_DEFAULT_ACCESS_LEVEL = AccessLevel.ADMIN  # Outside XFEL

KARABO_FRAMEWORK_VERSION = "1.1.2"

MAX_INT32 = (2**31)-1 # 0x7fffffff
MIN_INT32 = -(2**31) # -0x80000000