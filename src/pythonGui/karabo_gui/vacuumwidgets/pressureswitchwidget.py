#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 11, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class VacuumWidget.
"""

from karabo_gui.widget import VacuumWidget


class PressureSwitchWidget(VacuumWidget):
    alias = "Pressure switch"

    def valueChanged(self, box, value, timestamp=None):
        if value.endswith('ING'):
            self._setPixmap("pressure-switch-orange")
        elif value == 'ON':
            self._setPixmap("pressure-switch-green")
        elif value == 'OFF':
            self._setPixmap("pressure-switch-yellow")
        elif value == 'ERROR':
            self._setPixmap("pressure-switch-red")
        else:
            self._setPixmap("pressure-switch")
