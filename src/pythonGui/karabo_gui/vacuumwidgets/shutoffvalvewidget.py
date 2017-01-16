#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 11, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class VacuumWidget.
"""

from karabo.common.states import State
from karabo_gui.widget import VacuumWidget


class ShutOffValveWidget(VacuumWidget):
    alias = "Shut off valve"
    statePixmapName = {
        State.CHANGING: 'shutoff-valve-orange',
        State.ACTIVE: 'shutoff-valve-green',
        State.PASSIVE: 'shutoff-valve-yellow',
        State.NORMAL: 'shutoff-valve-yellow',
        State.ERROR: 'shutoff-valve-red',
        State.UNKNOWN: 'shutoff-valve'
    }
