# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$May 10, 2013 2:35:08 PM$"

from karabo_decorators import *
from base_fsm import *
from fsm import *

@KARABO_CLASSINFO("StartStopFsm", "1.0")
class StartStopFsm(BaseFsm):
    
    @staticmethod
    def expectedParameters(expected):
        
        e = SLOT_ELEMENT(expected).key("start")
        e.displayedName("Start").description("Instructs device to go to started state")
        e.allowedStates("Ok.Stopped")
        e.commit()

        e = SLOT_ELEMENT(expected).key("stop")
        e.displayedName("Stop").description("Instructs device to go to stopped state")
        e.allowedStates("Ok.Started")
        e.commit()

        e = SLOT_ELEMENT(expected).key("reset")
        e.displayedName("Reset").description("Resets the device in case of an error")
        e.allowedStates("Error")
        e.commit()

    def __init__(self, configuration):
        super(StartStopFsm, self).__init__(configuration)
        
        #**************************************************************
        #*                        Events                              *
        #**************************************************************
        KARABO_FSM_EVENT2(self, 'ErrorFoundEvent', 'errorFound')
        KARABO_FSM_EVENT0(self, 'ResetEvent', 'reset')
        KARABO_FSM_EVENT0(self, 'StartEvent', 'start')
        KARABO_FSM_EVENT0(self, 'StopEvent',  'stop')

        #**************************************************************
        #*                        States                              *
        #**************************************************************
        KARABO_FSM_STATE_EE('Error',   self.errorStateOnEntry,   self.errorStateOnExit)
        KARABO_FSM_STATE_EE('Initialization', self.initializationStateOnEntry, self.initializationStateOnExit)
        KARABO_FSM_STATE_EE('Started', self.startedStateOnEntry, self.startedStateOnExit)
        KARABO_FSM_STATE_EE('Stopped', self.stoppedStateOnEntry, self.stoppedStateOnExit)

        #**************************************************************
        #*                    Transition Actions                      *
        #**************************************************************
        KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)
        KARABO_FSM_ACTION2('ErrorFoundAction', self.errorFoundAction, str, str)
        KARABO_FSM_ACTION0('ResetAction', self.resetAction)
        KARABO_FSM_ACTION0('StartAction', self.startAction)
        KARABO_FSM_ACTION0('StopAction',  self.stopAction)

        #**************************************************************
        #*                      Ok State Machine                      *
        #**************************************************************

        okStateTransitionTable = [
        # Source-State      Event    Target-State    Action     Guard
            ('Stopped', 'StartEvent', 'Started', 'StartAction', 'none'),
            ('Started', 'StopEvent',  'Stopped', 'StopAction',  'none')
        ]

        #                        Name     Transition-Table   Initial-State
        KARABO_FSM_STATE_MACHINE('Ok', okStateTransitionTable, 'Stopped')

        #**************************************************************
        #*                      Top Machine                           *
        #**************************************************************

        #  Source-State    Event     Target-State  Action          Guard
        startStopMachineTransitionTable = [
            ('Initialization', 'none', 'Ok',   'none',             'none'),
            ('Ok', 'ErrorFoundEvent', 'Error', 'ErrorFoundAction', 'none'),
            ('Error', 'ResetEvent',    'Ok',   'none',             'none')
        ]

        #                               Name                Transition-Table           Initial-State
        KARABO_FSM_STATE_MACHINE('StartStopMachine', startStopMachineTransitionTable, 'Initialization')
        self.fsm = KARABO_FSM_CREATE_MACHINE('StartStopMachine')
    
    def getFsm(self):
        return self.fsm
    
    def initFsmSlots(self, sigslot):
        sigslot.registerSlot(self.start)
        sigslot.registerSlot(self.stop)
        sigslot.registerSlot(self.reset)
