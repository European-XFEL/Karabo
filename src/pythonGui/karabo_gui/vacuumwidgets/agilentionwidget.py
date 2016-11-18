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


class AgilentIonWidget(VacuumWidget):
    alias = "Agilent Ion Pump"
    statePixmapName = {
        State.CHANGING: 'agilent-pump-changing',
        State.ACTIVE: 'agilent-pump-active',
        State.PASSIVE: 'agilent-pump-passive',
        State.INIT: 'agilent-pump-init',
        State.KNOWN: 'agilent-pump-known',
        State.ERROR: 'agilent-pump-error',
        State.UNKNOWN: 'agilent-pump-unknown',
        State.DISABLED: 'agilent-pump-disabled'
    }
