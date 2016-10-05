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


class PressureGaugeWidget(VacuumWidget):
    alias = "Pressure gauge"
    statePixmapName = {
        State.CHANGING: 'pressure-gauge-orange',
        State.ACTIVE: 'pressure-gauge-green',
        State.PASSIVE: 'pressure-gauge-yellow',
        State.ERROR: 'pressure-gauge-red',
        State.UNKNOWN: 'pressure-gauge'
    }
