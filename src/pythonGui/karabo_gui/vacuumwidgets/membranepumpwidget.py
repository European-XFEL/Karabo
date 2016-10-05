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


class MembranePumpWidget(VacuumWidget):
    alias = "Membrane Pump"
    statePixmapName = {
        State.CHANGING: 'membrane-pump-orange',
        State.ACTIVE: 'membrane-pump-green',
        State.PASSIVE: 'membrane-pump-yellow',
        State.ERROR: 'membrane-pump-red',
        State.UNKNOWN: 'membrane-pump'
    }
