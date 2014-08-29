#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 11, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class VacuumWidget.
"""

__all__ = ["ShutOffValveWidget"]


from widget import VacuumWidget


class ShutOffValveWidget(VacuumWidget):
    alias = "Shut off valve"
    
    def valueChanged(self, box, value, timestamp=None):
        if value == "Changing..." or ("TurningOnOrOpening" in value) or ("TurningOffOrClosing" in value):
            self._setPixmap("shutoff-valve-orange")
        elif ("On" in value) or ("on" in value):
            self._setPixmap("shutoff-valve-green")
        elif ("Off" in value) or ("off" in value):
            self._setPixmap("shutoff-valve-yellow")
        elif ("Error" in value) or ("error" in value):
            self._setPixmap("shutoff-valve-red")
        else:
            self._setPixmap("shutoff-valve")
