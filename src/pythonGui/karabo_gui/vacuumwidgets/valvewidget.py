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


class ValveWidget(VacuumWidget):
    alias = "Valve"
    statePixmapName = {
        State.CHANGING: 'valve-orange',
        State.ACTIVE: 'valve-green',
        State.PASSIVE: 'valve-yellow',
        State.ERROR: 'valve-red',
        State.UNKNOWN: 'valve'
    }
