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


class TurboPumpWidget(VacuumWidget):
    alias = "Turbo pump"
    statePixmapName = {
        State.CHANGING: 'turbo-pump-orange',
        State.ACTIVE: 'turbo-pump-green',
        State.PASSIVE: 'turbo-pump-yellow',
        State.ERROR: 'turbo-pump-red',
        State.UNKNOWN: 'turbo-pump'
    }
