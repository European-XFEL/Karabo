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
__date__ = "$May 10, 2013 2:17:13 PM$"

import threading

from karabo.common.states import State

from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from .fsm import KARABO_FSM_NO_TRANSITION_ACTION
from .no_fsm import NoFsm


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("BaseFsm", "1.0")
class BaseFsm(NoFsm):
    """The BaseFSM class is the basis of all finite state machines.

    It provides methods for starting and retrieving the derived specific
    state machines, and for processing events occurring in the state machine.
    """

    @staticmethod
    def expectedParameters(expected):
        """Expected parameters of the FSM.

        Derived state machines will usually implement slots in their
        expected parameters.

        :param expected: schema to insert parameters into
        """

        pass

    def __init__(self, configuration):
        """Initialize the state machine according to the configuration Hash.

        :param configuration:
        """
        super().__init__(configuration)
        self.fsm = None
        self.processEventLock = threading.RLock()
        KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)

    def getFsm(self):
        """
        Return the concrete FSM implementation that inherits from BaseFsm
        """
        return self.fsm

    def startFsm(self):
        """Start state machine"""
        fsm = self.getFsm()
        if fsm is not None:
            try:
                fsm.start()
            except Exception as e:
                raise RuntimeError(f"startFsm -- Exception: {str(e)}")

            state = fsm.get_state()
            self.updateState(state)

    def processEvent(self, event):
        """Process input event, i.e. drive state machine to the next state."""
        with self.processEventLock:
            fsm = self.getFsm()
            if fsm is not None:
                self.updateState(State.CHANGING)
                try:
                    fsm.process_event(event)
                except Exception as e:
                    name = event.__class__.__name__
                    self.exceptionFound(
                       f"Exception while processing event '{name}': {e}")
                finally:
                    state = fsm.get_state()
                    self.updateState(state)

    def exceptionFound(self, userFriendlyMessage, detailedMessage):
        """Hook for when an exception is encountered.

        Catch exceptions where they can occur instead, e.g.
        when calling a slot or requesting a value!

        :param shortMessage: exception message
        :param detailedMessage: detailed exception message
        """
