#!/usr/bin/env python

__author__="__EMAIL__"
__date__ ="__DATE__"
__copyright__="Copyright (c) 2010-2013 European XFEL GmbH Hamburg. All rights reserved."

import time
import sys
import threading
from karabo.device import *
from karabo.fsm import *
from karabo.karathon import *

@KARABO_CLASSINFO("__CLASS_NAME__", "1.0")
class __CLASS_NAME__(PythonDevice, BaseFsm):
    
    @staticmethod
    def expectedParameters(expected):
        (
        
        SLOT_ELEMENT(expected).key("inject")
                .displayedName("Inject")
                .description("Injects parameters")
                .allowedStates("Ok.Uninjected")
                .commit(),
       
        
        SLOT_ELEMENT(expected).key("reset")
                .displayedName("Reset")
                .description("Resets the device in case of an error")
                .allowedStates("Error")
                .commit(),
        
        STRING_ELEMENT(expected).key("result")
                .displayedName("Result")
                .description("The resultant word from the injection")
                .readOnly()
                .commit(),
        )
        
       

    def __init__(self, configuration):
        # always call PythonDevice constructor first!
        super(__CLASS_NAME__,self).__init__(configuration)
        
        #**************************************************************
        #*                        Events                              *
        #**************************************************************
        KARABO_FSM_EVENT2(self, 'ErrorFoundEvent', 'errorFound')
        KARABO_FSM_EVENT0(self, 'ResetEvent', 'reset')
        KARABO_FSM_EVENT0(self, 'InjectEvent', 'inject')
        KARABO_FSM_EVENT0(self, 'UninjectEvent',  'uninject')
        

        #**************************************************************
        #*                        States                              *
        #**************************************************************
        KARABO_FSM_STATE('Uninjected')
        KARABO_FSM_STATE('Injected')
        KARABO_FSM_STATE('Error')
        

        #**************************************************************
        #*                    Transition Actions                      *
        #**************************************************************
        KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)
        KARABO_FSM_ACTION2('ErrorFoundAction', self.errorFoundAction, str, str)
        KARABO_FSM_ACTION0('InjectAction', self.injectAction)
        KARABO_FSM_ACTION0('UninjectAction',  self.uninjectAction)

        #**************************************************************
        #*                      Ok State Machine                      *
        #**************************************************************

        okStateTransitionTable = [
        # Source-State      Event    Target-State    Action     Guard
            ('Uninjected', 'InjectEvent', 'Injected', 'InjectAction', 'none'),
            ('Injected', 'UninjectEvent',  'Uninjected', 'UninjectAction',  'none')
        ]

        #                        Name     Transition-Table   Initial-State
        KARABO_FSM_STATE_MACHINE('Ok', okStateTransitionTable, 'Uninjected')

        #**************************************************************
        #*                      Top Machine                           *
        #**************************************************************

        #  Source-State    Event     Target-State  Action          Guard
        stateMachineTransitionTable = [
            ('Ok', 'ErrorFoundEvent', 'Error', 'ErrorFoundAction', 'none'),
            ('Error', 'ResetEvent',    'Ok',   'none',             'none')
        ]

        #                               Name                Transition-Table           Initial-State
        KARABO_FSM_STATE_MACHINE('StateMachine', stateMachineTransitionTable, 'Ok')
        self.fsm = KARABO_FSM_CREATE_MACHINE('StateMachine')
        
        self._ss.registerSlot(self.inject)
        self._ss.registerSlot(self.uninject)
        self._ss.registerSlot(self.reset)
                        
    def getFsm(self):
        return self.fsm
    
    
    def initFsmSlots(self, sigslot):
        pass
        
    ##############################################
    #   Implementation of State Machine methods  #
    ##############################################
        
    def injectAction(self):
        
        schema = Schema()
        
        (
        SLOT_ELEMENT(schema).key("uninject")
                .displayedName("Uninject")
                .description("Uninjects parameters")
                .allowedStates("Ok.Injected")
                .commit(),
        
        STRING_ELEMENT(schema).key("word")
                .displayedName("Word")
                .description("The word")
                .assignmentOptional().defaultValue("Hello")
                .reconfigurable()
                .commit(),
        )
        self.updateSchema(schema)
        
        
    def uninjectAction(self):
        self.set("result", self.get("word"))
        self.updateSchema(Schema())
        
       
# This entry used by device server
if __name__ == "__main__":
    launchPythonDevice()
