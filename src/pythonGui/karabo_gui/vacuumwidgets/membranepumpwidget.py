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

    def valueChanged(self, box, value, timestamp=None):
        if State(value).isDerivedFrom(State.CHANGING):
            self._setPixmap("membrane-pump-orange")
        elif State(value).isDerivedFrom(State.ACTIVE):
            self._setPixmap("membrane-pump-green")
        elif State(value).isDerivedFrom(State.PASSIVE):
            self._setPixmap("membrane-pump-yellow")
        elif State(value) is State.ERROR:
            self._setPixmap("membrane-pump-red")
        else:
            self._setPixmap("membrane-pump")
