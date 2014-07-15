#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 11, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class VacuumWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["TemperatureProbeWidget"]


from widget import VacuumWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class TemperatureProbeWidget(VacuumWidget):
    alias = "Temperature probe"

    def valueChanged(self, box, value, timestamp=None):
        if value == "Changing...":
            self._setPixmap("thermometer-orange")
        elif ("On" in value) or ("on" in value):
            self._setPixmap("thermometer-green")
        elif ("Off" in value) or ("off" in value):
            self._setPixmap("thermometer-yellow")
        elif ("Error" in value) or ("error" in value):
            self._setPixmap("thermometer-red")
        else:
            self._setPixmap("thermometer")
