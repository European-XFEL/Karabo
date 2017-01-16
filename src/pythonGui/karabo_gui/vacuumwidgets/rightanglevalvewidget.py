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


class RightAngleValveWidget(VacuumWidget):
    alias = "Right angle valve"
    statePixmapName = {
        State.CHANGING: 'rightangle-valve-orange',
        State.ACTIVE: 'rightangle-valve-green',
        State.PASSIVE: 'rightangle-valve-yellow',
        State.NORMAL: 'rightangle-valve-yellow',
        State.ERROR: 'rightangle-valve-red',
        State.UNKNOWN: 'rightangle-valve'
    }
