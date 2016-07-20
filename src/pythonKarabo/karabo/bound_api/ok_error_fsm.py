# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$May 12, 2013 5:09:05 PM$"

from karathon import SLOT_ELEMENT

import karabo.bound_api.base_fsm as base
from karabo.common.states import State
from .decorators import KARABO_CLASSINFO
from .fsm import (
    KARABO_FSM_ACTION0, KARABO_FSM_ACTION2,
    KARABO_FSM_EVENT0, KARABO_FSM_EVENT2,
    KARABO_FSM_STATE_EE, KARABO_FSM_CREATE_MACHINE, KARABO_FSM_STATE_MACHINE
)


@KARABO_CLASSINFO("OkErrorFsm", "1.0")
class OkErrorFsm(base.BaseFsm):
    
    @staticmethod
    def expectedParameters(expected):
        
        e = SLOT_ELEMENT(expected).key("reset")
        e.displayedName("Reset").description("Resets the device in case of an error")
        e.allowedStates(State.ERROR)
        e.commit()

    def __init__(self, configuration):
        super(OkErrorFsm, self).__init__(configuration)
        
        #**************************************************************
        #*                        Events                              *
        #**************************************************************
        KARABO_FSM_EVENT2(self, 'ErrorFoundEvent', 'errorFound')
        KARABO_FSM_EVENT0(self, 'ResetEvent', 'reset')

        #**************************************************************
        #*                        States                              *
        #**************************************************************
        KARABO_FSM_STATE_EE(State.ERROR,   self.errorStateOnEntry,   self.errorStateOnExit)
        KARABO_FSM_STATE_EE(State.NORMAL,      self.okStateOnEntry,      self.okStateOnExit)

        #**************************************************************
        #*                    Transition Actions                      *
        #**************************************************************
        #KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)    # <-- this is defined in BaseFsm
        KARABO_FSM_ACTION2('ErrorFoundAction', self.errorFoundAction, str, str)
        KARABO_FSM_ACTION0('ResetAction', self.resetAction)

        #**************************************************************
        #*                      Top Machine                           *
        #**************************************************************

        #  Source-State    Event     Target-State  Action          Guard
        stateMachineTransitionTable = [
            (State.NORMAL, 'ErrorFoundEvent', State.ERROR, 'ErrorFoundAction', 'none'),
            (State.ERROR, 'ResetEvent', State.NORMAL,   'ResetAction',             'none')
        ]

        #                           Name                Transition-Table  Initial-State
        KARABO_FSM_STATE_MACHINE('StateMachine', stateMachineTransitionTable, State.NORMAL)
        self.fsm = KARABO_FSM_CREATE_MACHINE('StateMachine')
    
    def getFsm(self):
        return self.fsm
    
    def initFsmSlots(self, sigslot):
        sigslot.registerSlot(self.reset)
        sigslot.registerSlot(self.errorFound)
        
    def errorStateOnEntry(self):
        '''Actions executed on entry to 'Error' state
        '''
        
    def errorStateOnExit(self):
        '''Actions executed on exit from 'Error' state
        '''
    
    def errorFoundAction(self, m1, m2):
        print("***", m1, m2)
        
    def okStateOnEntry(self):
        '''Actions executed on entry to 'Ok' state
        '''
        
    def okStateOnExit(self):
        '''Actions executed on exit from 'Ok' state
        '''

    def resetAction(self):
        '''Reset action on the way to 'Ok' dtate
        '''
