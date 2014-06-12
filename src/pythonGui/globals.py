#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 23, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from enums import AccessLevel
from os import environ, path
from sys import platform

# TODO Karabo will support an global access level and an excpetion list which is deviceId specific
# This requires a function like in SignalSlotable: getAccessLevel(deviceId) in the end


GLOBAL_ACCESS_LEVEL = AccessLevel.OBSERVER
#KARABO_DEFAULT_ACCESS_LEVEL = AccessLevel.OBSERVER  # Inside XFEL
KARABO_DEFAULT_ACCESS_LEVEL = AccessLevel.ADMIN  # Outside XFEL

# Hidden karabo folder which includes certain karabo related files
if "win" in platform:
    HIDDEN_KARABO_FOLDER = path.join(environ['APPDATA'], "karabo")
else:
    HIDDEN_KARABO_FOLDER = path.join(environ['HOME'], ".karabo")
# Project folder
KARABO_PROJECT_FOLDER = path.join(HIDDEN_KARABO_FOLDER, "projects")


MAX_INT8 = (2**7)-1
MIN_INT8 = -(2**7)

MAX_UINT8 = (2**8)-1

MAX_INT16 = (2**15)-1
MIN_INT16 = -(2**15)

MAX_UINT16 = (2**16)-1

MAX_INT32 = (2**31)-1 # 0x7fffffff
MIN_INT32 = -(2**31) # -0x80000000

MAX_UINT32 = (2**32)-1 # 0xffffffff

MAX_INT64 = (2**63)-1
MIN_INT64 = -(2**63)

MAX_UINT64 = (2**64)-1

