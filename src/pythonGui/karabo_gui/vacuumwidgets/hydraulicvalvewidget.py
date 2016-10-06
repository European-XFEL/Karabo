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
    statePixmapName = {
        State.CHANGING: 'hydraulic-valve-orange',
        State.ACTIVE: 'hydraulic-valve-green',
        State.PASSIVE: 'hydraulic-valve-yellow',
        State.ERROR: 'hydraulic-valve-red',
        State.UNKNOWN: 'hydraulic-valve'
    }
