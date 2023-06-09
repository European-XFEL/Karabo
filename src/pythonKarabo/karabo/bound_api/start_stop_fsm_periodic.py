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
__author__ = "Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ = "$May 10, 2013 2:35:08 PM$"

import karabo.bound_api.base_fsm as base
from karabo.common.states import State
from karathon import SLOT_ELEMENT

from .decorators import KARABO_CLASSINFO
from .fsm import (
    KARABO_FSM_ACTION0, KARABO_FSM_ACTION2, KARABO_FSM_CREATE_MACHINE,
    KARABO_FSM_EVENT0, KARABO_FSM_EVENT2, KARABO_FSM_PERIODIC_ACTION,
    KARABO_FSM_STATE_AEE, KARABO_FSM_STATE_EE, KARABO_FSM_STATE_MACHINE)


@KARABO_CLASSINFO("StartStopFsmPeriodic", "1.0")
class StartStopFsmPeriodic(base.BaseFsm):

    @staticmethod
    def expectedParameters(expected):
        e = SLOT_ELEMENT(expected).key("start")
        e.displayedName("Start").description(
            "Instructs device to go to started state")
        e.allowedStates(State.STOPPED)
        e.commit()

        e = SLOT_ELEMENT(expected).key("stop")
        e.displayedName("Stop").description(
            "Instructs device to go to stopped state")
        e.allowedStates(State.STARTED)
        e.commit()

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
        KARABO_FSM_EVENT0(self, 'StartEvent', 'start')
        KARABO_FSM_EVENT0(self, 'StopEvent', 'stop')

        # **************************************************************
        # *                        State Actions                       *
        # **************************************************************

        KARABO_FSM_PERIODIC_ACTION('PeriodicAction', 1000, -1,
                                   self.periodicAction)

        # **************************************************************
        # *                        States                              *
        # **************************************************************
        KARABO_FSM_STATE_EE(State.ERROR, self.errorStateOnEntry,
                            self.errorStateOnExit)
        KARABO_FSM_STATE_EE(State.INIT, self.initializationStateOnEntry,
                            self.initializationStateOnExit)
        KARABO_FSM_STATE_AEE(State.STARTED, 'PeriodicAction',
                             self.startedStateOnEntry, self.startedStateOnExit)
        KARABO_FSM_STATE_EE(State.STOPPED, self.stoppedStateOnEntry,
                            self.stoppedStateOnExit)

        # **************************************************************
        # *                    Transition Actions                      *
        # **************************************************************
        # KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)
        KARABO_FSM_ACTION2('ErrorFoundAction', self.errorFoundAction, str, str)
        KARABO_FSM_ACTION0('ResetAction', self.resetAction)
        KARABO_FSM_ACTION0('StartAction', self.startAction)
        KARABO_FSM_ACTION0('StopAction', self.stopAction)

        # **************************************************************
        # *                      Ok State Machine                      *
        # **************************************************************

        okStateTransitionTable = [
            # Source-State      Event    Target-State    Action     Guard
            (State.STOPPED, 'StartEvent', State.STARTED, 'StartAction',
             'none'),
            (State.STARTED, 'StopEvent', State.STOPPED, 'StopAction', 'none')
        ]

        #       Name     Transition-Table   Initial-State
        KARABO_FSM_STATE_MACHINE(State.NORMAL, okStateTransitionTable,
                                 State.STOPPED)

        # **************************************************************
        # *                      Top Machine                           *
        # **************************************************************

        #  Source-State    Event     Target-State  Action          Guard
        startStopMachineTransitionTable = [
            (State.INIT, 'none', State.NORMAL, 'none', 'none'),
            ('Ok', 'ErrorFoundEvent', State.ERROR, 'ErrorFoundAction', 'none'),
            (State.ERROR, 'ResetEvent', State.NORMAL, 'none', 'none')
        ]

        #      Name                Transition-Table           Initial-State
        KARABO_FSM_STATE_MACHINE('StartStopMachine',
                                 startStopMachineTransitionTable, State.INIT)
        self.fsm = KARABO_FSM_CREATE_MACHINE('StartStopMachine')

    def stop(self):
        self.fsm.stop()

    def getFsm(self):
        return self.fsm

    def initFsmSlots(self, sigslot):
        sigslot.registerSlot(self.start)
        sigslot.registerSlot(self.stop)
        sigslot.registerSlot(self.reset)
        sigslot.registerSlot(self.errorFound)

    def initializationStateOnEntry(self):
        """Actions executed on entry to 'Initialization' state"""

    def initializationStateOnExit(self):
        """Actions executed on exit from 'Initialization' state"""

    def errorStateOnEntry(self):
        """Actions executed on entry to 'Error' state"""

    def errorStateOnExit(self):
        """Actions executed on exit from 'Error' state"""

    def startedStateOnEntry(self):
        """Actions executed on entry to 'Started' state"""

    def startedStateOnExit(self):
        """Actions executed on exit from 'Started' state"""

    def stoppedStateOnEntry(self):
        """Actions executed on entry to 'Stopped' state"""

    def stoppedStateOnExit(self):
        """Actions executed on exit from 'Stopped' state"""

    def startAction(self):
        """Actions executed at 'start' event"""

    def stopAction(self):
        """Actions executed at 'stop' event"""

    def resetAction(self):
        print("Reset action executed")

    def periodicAction(self, expired):
        print("*** periodicAction called ***")
