#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 11, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class VacuumWidget.
"""

__all__ = ["TurboPumpWidget"]


from karabo_gui.widget import VacuumWidget


class TurboPumpWidget(VacuumWidget):
    alias = "Turbo pump"

    def valueChanged(self, box, value, timestamp=None):
        if value == "Changing...":
            self._setPixmap("turbo-pump-orange")
        elif ("On" in value) or ("on" in value):
            self._setPixmap("turbo-pump-green")
        elif ("Off" in value) or ("off" in value):
            self._setPixmap("turbo-pump-yellow")
        elif ("Error" in value) or ("error" in value):
            self._setPixmap("turbo-pump-red")
        else:
            self._setPixmap("turbo-pump")
