__author__="andrea.parenti@xfel.eu"
__date__ ="August  7, 2013"

import karabo.bound_api.base_fsm as base
from karathon import SLOT_ELEMENT
from .decorators import KARABO_CLASSINFO
from .fsm import (
    KARABO_FSM_EVENT0, KARABO_FSM_EVENT2,
    KARABO_FSM_ACTION0, KARABO_FSM_ACTION2,
    KARABO_FSM_GUARD0,
    KARABO_FSM_STATE_EE, KARABO_FSM_STATE_MACHINE,
    KARABO_FSM_CREATE_MACHINE)

from karabo.common.states import State


@KARABO_CLASSINFO("CameraFsm", "1.0")
class CameraFsm(base.BaseFsm):
    """
    The CameraFSM is a state machine suited for camera devices.

    This finite state machine implements actions for acquiring, triggering
    stopping and resetting camera type devices.

    Here acquire sends the camera into the acquisition state, i.e. it is ready
    to receive and react on trigger signals. The trigger command will send
    such a trigger. Stop will stop acquisition, i.e. the camera will not
    respond to triggers anymore. Finally, reset is used to bring the camera
    back into an active state after hardware errors have occured.
    """

    @staticmethod
    def expectedParameters(expected):
        (
            SLOT_ELEMENT(expected).key("connectCamera")
                .displayedName("Connect")
                .description("Connects to the hardware")
                .allowedStates(State.UNKNOWN)
                .commit(),

            SLOT_ELEMENT(expected).key("acquire")
                .displayedName("Acquire")
                .description("Instructs camera to go into acquisition state")
                .allowedStates(State.ACTIVE)
                .commit(),

            SLOT_ELEMENT(expected).key("trigger")
                .displayedName("Trigger")
                .description("Sends a software trigger to the camera")
                .allowedStates(State.ACQUIRING)
                .commit(),

            SLOT_ELEMENT(expected).key("stop")
                .displayedName("Stop")
                .description("Instructs camera to stop current acquisition")
                .allowedStates(State.ACQUIRING)
                .commit(),

            SLOT_ELEMENT(expected).key("reset")
                .displayedName("Reset")
                .description("Resets the camera in case of an error")
                .allowedStates(State.ERROR)
                .commit()
        )

    def __init__(self, configuration):
        super(CameraFsm, self).__init__(configuration)

        #**************************************************************
        #*                        Events                              *
        #**************************************************************
        KARABO_FSM_EVENT2(self, 'ErrorFoundEvent', 'errorFound')
        KARABO_FSM_EVENT0(self, 'ConnectEvent',    'connectCamera')
        KARABO_FSM_EVENT0(self, 'DisconnectEvent', 'disconnectCamera')
        KARABO_FSM_EVENT0(self, 'ResetEvent',      'reset')
        KARABO_FSM_EVENT0(self, 'AcquireEvent',    'acquire')
        KARABO_FSM_EVENT0(self, 'StopEvent',       'stop')
        KARABO_FSM_EVENT0(self, 'TriggerEvent',    'trigger')        

        #**************************************************************
        #*                        States                              *
        #**************************************************************
        KARABO_FSM_STATE_EE(State.ERROR, self.errorStateOnEntry, self.errorStateOnExit)
        KARABO_FSM_STATE_EE(State.INIT, self.initializationStateOnEntry, self.initializationStateOnExit)
        KARABO_FSM_STATE_EE(State.UNKNOWN, self.unknownStateOnEntry, self.unknownStateOnExit)
        KARABO_FSM_STATE_EE(State.ACQUIRING, self.acquisitionStateOnEntry, self.acquisitionStateOnExit)
        KARABO_FSM_STATE_EE(State.ACTIVE, self.readyStateOnEntry, self.readyStateOnExit)

        #**************************************************************
        #*                    Transition Actions                      *
        #**************************************************************
        #KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)    
        KARABO_FSM_ACTION2('ErrorFoundAction', self.errorFoundAction, str, str)
        KARABO_FSM_ACTION0('ConnectAction', self.connectAction)
        KARABO_FSM_ACTION0('DisconnectAction', self.disconnectAction)
        KARABO_FSM_ACTION0('ResetAction', self.resetAction)
        KARABO_FSM_ACTION0('AcquireAction', self.acquireAction)
        KARABO_FSM_ACTION0('StopAction',  self.stopAction)
        KARABO_FSM_ACTION0('TriggerAction',  self.triggerAction)

        KARABO_FSM_GUARD0('ConnectGuard', self.connectGuard)

        #**************************************************************
        #*                       Ok State Machine                     *
        #**************************************************************
        okStt=[
        # Source-State    Event           Target-State     Action           Guard
        (State.ACTIVE,    'AcquireEvent', State.ACQUIRING, 'AcquireAction', 'none'),
        (State.ACQUIRING, 'StopEvent',    State.ACTIVE,    'StopAction',    'none'),
        (State.ACQUIRING, 'TriggerEvent', None,            'TriggerAction', 'none')
        ]
        #                        Name  Transition-Table  Initial-State
        KARABO_FSM_STATE_MACHINE(State.NORMAL, okStt, State.ACTIVE)

        #**************************************************************
        #*                      Known Machine                         *
        #**************************************************************
        knownStt=[
        # Source-State   Event             Target-State   Action              Guard
        (State.NORMAL,  'ErrorFoundEvent', State.ERROR,   'ErrorFoundAction', 'none'),
        (State.ERROR,   'ResetEvent',      State.NORMAL,  'none',             'none')
        ]
        #                         Name      Transition-Table  Initial-State
        KARABO_FSM_STATE_MACHINE(State.KNOWN, knownStt, State.NORMAL)

        #**************************************************************
        #*                       Top Machine                          *
        #**************************************************************
        cameraStt=[
        # Source-State   Event             Target-State   Action               Guard
        (State.INIT,    'none',            State.UNKNOWN, 'none',             'none'),
        (State.UNKNOWN, 'ConnectEvent',    State.KNOWN,   'ConnectAction',    'ConnectGuard'),
        (State.KNOWN,   'DisconnectEvent', State.UNKNOWN, 'DisconnectAction', 'none')
        ]
        #                         Name      Transition-Table  Initial-State
        KARABO_FSM_STATE_MACHINE('CameraMachine', cameraStt, State.INIT)
        self.fsm = KARABO_FSM_CREATE_MACHINE('CameraMachine')

    def getFsm(self):
        return self.fsm

    def initFsmSlots(self, sigslot):
        sigslot.registerSlot(self.connectCamera)
        sigslot.registerSlot(self.acquire)
        sigslot.registerSlot(self.trigger)
        sigslot.registerSlot(self.stop)
        sigslot.registerSlot(self.reset)
        sigslot.registerSlot(self.errorFound)

    def initializationStateOnEntry(self):
        """Actions executed on entry to 'Initialization' state"""

    def initializationStateOnExit(self):
        """Actions executed on exit from 'Initialization' state"""

    def unknownStateOnEntry(self):
        """Actions executed on entry to 'Initialization' state"""

    def unknownStateOnExit(self):
        """Actions executed on exit from 'Initialization' state"""

    def errorStateOnEntry(self):
        """Actions executed on entry to 'Error' state"""

    def errorStateOnExit(self):
        """Actions executed on exit from 'Error' state"""

    def acquisitionStateOnEntry(self):
        """Actions executed on entry to 'Acquisition' state"""

    def acquisitionStateOnExit(self):
        """Actions executed on exit from 'Acquisition' state"""

    def readyStateOnEntry(self):
        """Actions executed on entry to 'Ready' state"""

    def readyStateOnExit(self):
        """Actions executed on exit from 'Ready' state"""

    def connectAction(self):
        """Actions executed at 'connect' event"""

    def disconnectAction(self):
        """Actions executed at 'disconnect' event"""

    def acquireAction(self):
        """Actions executed at 'acquire' event"""

    def stopAction(self):
        """Actions executed at 'stop' event"""

    def triggerAction(self):
        """Actions executed at 'trigger' event"""

    def resetAction(self):
        """Action to execute upon reset event"""

    def connectGuard(self):
        """Guard for UNKNOWN -> KNOWN transition"""
