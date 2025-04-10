#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 11, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class VacuumWidget.
"""

__all__ = ["PressureGaugeWidget"]


from karabo_gui.widget import VacuumWidget


class PressureGaugeWidget(VacuumWidget):
    alias = "Pressure gauge"

    def valueChanged(self, box, value, timestamp=None):
        if value == "Changing...":
            self._setPixmap("pressure-gauge-orange")
        elif ("On" in value) or ("on" in value) or ("AllOk" == value):
            self._setPixmap("pressure-gauge-green")
        elif ("Off" in value) or ("off" in value):
            self._setPixmap("pressure-gauge-yellow")
        elif ("Error" in value) or ("error" in value):
            self._setPixmap("pressure-gauge-red")
        else:
            self._setPixmap("pressure-gauge")
