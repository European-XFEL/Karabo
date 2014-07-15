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

__all__ = ["MotorWidget"]


from widget import VacuumWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class MotorWidget(VacuumWidget):
    alias = "Motor"

    def valueChanged(self, box, value, timestamp=None):
        if value == "Changing..." or ("Moving" in value):
            self._setPixmap("motor-orange")
        elif ("On" in value) or ("on" in value) or ("Idle" in value):
            self._setPixmap("motor-green")
        elif ("Off" in value) or ("off" in value):
            self._setPixmap("motor-yellow")
        elif ("Error" in value) or ("error" in value):
            self._setPixmap("motor-red")
        else:
            self._setPixmap("motor")
