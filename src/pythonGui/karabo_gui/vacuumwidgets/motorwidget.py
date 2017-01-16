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


class MotorWidget(VacuumWidget):
    alias = "Motor"
    statePixmapName = {
        State.CHANGING: 'motor-orange',
        State.ACTIVE: 'motor-green',
        State.PASSIVE: 'motor-yellow',
        State.NORMAL: 'motor-yellow',
        State.ERROR: 'motor-red',
        State.UNKNOWN: 'motor'
    }
