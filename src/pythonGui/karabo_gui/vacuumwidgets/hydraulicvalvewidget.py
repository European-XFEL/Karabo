#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 25, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class VacuumWidget.
"""

__all__ = ["HydraulicValveWidget"]


from karabo_gui.widget import VacuumWidget


class HydraulicValveWidget(VacuumWidget):
    alias = "Hydraulic valve"

    def valueChanged(self, box, value, timestamp=None):
        if value.endswith('ING'):
            self._setPixmap("hydraulic-valve-orange")
        elif value == 'ON':
            self._setPixmap("hydraulic-valve-green")
        elif value == 'OFF':
            self._setPixmap("hydraulic-valve-yellow")
        elif value == 'ERROR':
            self._setPixmap("hydraulic-valve-red")
        else:
            self._setPixmap("hydraulic-valve")
