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

    def valueChanged(self, box, value, timestamp=None):
        if State(value).isDerivedFrom(State.CHANGING):
            self._setPixmap("shutoff-valve-orange")
        elif State(value).isDerivedFrom(State.ACTIVE):
            self._setPixmap("shutoff-valve-green")
        elif State(value).isDerivedFrom(State.PASSIVE):
            self._setPixmap("shutoff-valve-yellow")
        elif State(value) is State.ERROR:
            self._setPixmap("shutoff-valve-red")
        else:
            self._setPixmap("shutoff-valve")
