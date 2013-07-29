#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 23, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from karabo.karathon import AccessLevel

# TODO Karabo will support an global access level and an excpetion list which is deviceId specific
# This requires a function like in SignalSlotable: getAccessLevel(deviceId) in the end

def init():
    
    global GLOBAL_ACCESS_LEVEL
    GLOBAL_ACCESS_LEVEL = AccessLevel.OBSERVER
    
    global KARABO_DEFAULT_ACCESS_LEVEL
    KARABO_DEFAULT_ACCESS_LEVEL = AccessLevel.OBSERVER  # Inside XFEL
    #KARABO_DEFAULT_ACCESS_LEVEL = AccessLevel.ADMIN  # Outside XFEL
    