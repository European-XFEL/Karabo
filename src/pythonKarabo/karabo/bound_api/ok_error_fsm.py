# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__ = "Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ = "$May 12, 2013 5:09:05 PM$"

import karabo.bound_api.base_fsm as base
from karabo.bound_tool import SLOT_ELEMENT
from karabo.common.states import State

from .decorators import KARABO_CLASSINFO
from .fsm import (
    KARABO_FSM_ACTION0, KARABO_FSM_ACTION2, KARABO_FSM_CREATE_MACHINE,
    KARABO_FSM_EVENT0, KARABO_FSM_EVENT2, KARABO_FSM_STATE_EE,
    KARABO_FSM_STATE_MACHINE)


@KARABO_CLASSINFO("OkErrorFsm", "1.0")
class OkErrorFsm(base.BaseFsm):

    @staticmethod
    def expectedParameters(expected):
        e = SLOT_ELEMENT(expected).key("reset")
        e.displayedName("Reset").description(
            "Resets the device in case of an error")
        e.allowedStates(State.ERROR)
        e.commit()

    def __init__(self, configuration):
        super().__init__(configuration)

        # **************************************************************
        # *                        Events                              *
        # **************************************************************
        KARABO_FSM_EVENT2(self, 'ErrorFoundEvent', 'errorFound')
        KARABO_FSM_EVENT0(self, 'ResetEvent', 'reset')

        # **************************************************************
        # *                        States                              *
        # **************************************************************
        KARABO_FSM_STATE_EE(State.ERROR, self.errorStateOnEntry,
                            self.errorStateOnExit)
        KARABO_FSM_STATE_EE(State.NORMAL, self.okStateOnEntry,
                            self.okStateOnExit)

        # **************************************************************
        # *                    Transition Actions                      *
        # **************************************************************
        # This is defined in BaseFsm
        # KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)
        KARABO_FSM_ACTION2('ErrorFoundAction', self.errorFoundAction, str, str)
        KARABO_FSM_ACTION0('ResetAction', self.resetAction)

        # **************************************************************
        # *                      Top Machine                           *
        # **************************************************************

        stateMachineTransitionTable = [
            (State.NORMAL, 'ErrorFoundEvent', State.ERROR,
             'ErrorFoundAction', 'none'),
            (State.ERROR, 'ResetEvent', State.NORMAL,
             'ResetAction', 'none')]

        KARABO_FSM_STATE_MACHINE('StateMachine', stateMachineTransitionTable,
                                 State.NORMAL)
        self.fsm = KARABO_FSM_CREATE_MACHINE('StateMachine')

    def getFsm(self):
        return self.fsm

    def initFsmSlots(self, sigslot):
        sigslot.registerSlot(self.reset)
        sigslot.registerSlot(self.errorFound)

    def errorStateOnEntry(self):
        """Actions executed on entry to 'Error' state"""

    def errorStateOnExit(self):
        """Actions executed on exit from 'Error' state"""

    def errorFoundAction(self, m1, m2):
        print("***", m1, m2)

    def okStateOnEntry(self):
        """Actions executed on entry to 'Ok' state"""

    def okStateOnExit(self):
        """Actions executed on exit from 'Ok' state"""

    def resetAction(self):
        """Reset action on the way to 'Ok' dtate"""
