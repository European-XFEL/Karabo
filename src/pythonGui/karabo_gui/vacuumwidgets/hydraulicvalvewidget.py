#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 25, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class VacuumWidget.
"""

from karabo.common.states import State
from karabo_gui.widget import VacuumWidget


class HydraulicValveWidget(VacuumWidget):
    alias = "Hydraulic valve"

    def valueChanged(self, box, value, timestamp=None):
        if State(value).isDerivedFrom(State.CHANGING):
            self._setPixmap("hydraulic-valve-orange")
        elif State(value).isDerivedFrom(State.ACTIVE):
            self._setPixmap("hydraulic-valve-green")
        elif State(value).isDerivedFrom(State.PASSIVE):
            self._setPixmap("hydraulic-valve-yellow")
        elif State(value) is State.ERROR:
            self._setPixmap("hydraulic-valve-red")
        else:
            self._setPixmap("hydraulic-valve")
