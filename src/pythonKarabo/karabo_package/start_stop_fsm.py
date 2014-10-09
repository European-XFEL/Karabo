# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$May 10, 2013 2:35:08 PM$"

from karabo.decorators import KARABO_CLASSINFO
import karabo.base_fsm as base
from karabo.fsm import *
from karabo.hashtypes import Slot

@KARABO_CLASSINFO("StartStopFsm", "1.0")
class StartStopFsm(base.BaseFsm):
    start = Slot(
        displayedName="Start",
        description="Instructs device to go to started state",
        allowedStates="Ok.Stopped")

    stop = Slot(
        displayedName="Stop",
        description="Instructs device to go to stopped state",
        allowedStates="Ok.Started")

    reset = Slot(
        displayedName="Reset",
        description="Resets the device in case of an error",
        allowedStates="Error")

    def __init__(self, configuration):
        super(StartStopFsm, self).__init__(configuration)
        
        #**************************************************************
        #*                        Events                              *
        #**************************************************************
        KARABO_FSM_EVENT(self, 'ErrorFoundEvent', 'errorFound')
        KARABO_FSM_EVENT(self, 'ResetEvent', 'reset')
        KARABO_FSM_EVENT(self, 'StartEvent', 'start')
        KARABO_FSM_EVENT(self, 'StopEvent',  'stop')

        #**************************************************************
        #*                        States                              *
        #**************************************************************
        KARABO_FSM_STATE('Error',   self.errorStateOnEntry,   self.errorStateOnExit)
        KARABO_FSM_STATE('Initialization', self.initializationStateOnEntry, self.initializationStateOnExit)
        KARABO_FSM_STATE('Started', self.startedStateOnEntry, self.startedStateOnExit)
        KARABO_FSM_STATE('Stopped', self.stoppedStateOnEntry, self.stoppedStateOnExit)

        #**************************************************************
        #*                    Transition Actions                      *
        #**************************************************************
        #KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)
        KARABO_FSM_ACTION('ErrorFoundAction', self.errorFoundAction, str, str)
        KARABO_FSM_ACTION('ResetAction', self.resetAction)
        KARABO_FSM_ACTION('StartAction', self.startAction)
        KARABO_FSM_ACTION('StopAction',  self.stopAction)

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
    

    def initializationStateOnEntry(self):
        '''Actions executed on entry to 'Initialization' state
        '''
        
    def initializationStateOnExit(self):
        '''Actions executed on exit from 'Initialization' state
        '''
        
    def errorStateOnEntry(self):
        '''Actions executed on entry to 'Error' state
        '''
        
    def errorStateOnExit(self):
        '''Actions executed on exit from 'Error' state
        '''
        
    def startedStateOnEntry(self):
        '''Actions executed on entry to 'Started' state
        '''
        
    def startedStateOnExit(self):
        '''Actions executed on exit from 'Started' state
        '''
        
    def stoppedStateOnEntry(self):
        '''Actions executed on entry to 'Stopped' state
        '''
        
    def stoppedStateOnExit(self):
        '''Actions executed on exit from 'Stopped' state
        '''
        
    def startAction(self):
        '''Actions executed at 'start' event'''
        
    def stopAction(self):
        '''Actions executed at 'stop' event'''
        
    def resetAction(self):
        print("Reset action executed")
        