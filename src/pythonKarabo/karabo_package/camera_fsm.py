# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="andrea.parenti@xfel.eu"
__date__ ="August  7, 2013"

from karabo.decorators import KARABO_CLASSINFO
import karabo.base_fsm as base
from karabo.karathon import SLOT_ELEMENT
from karabo.fsm import *

@KARABO_CLASSINFO("CameraFsm", "1.0")
class CameraFsm(base.BaseFsm):
    
    @staticmethod
    def expectedParameters(expected):
        (
        SLOT_ELEMENT(expected).key("acquire")
        .displayedName("Acquire")
        .description("Instructs camera to go into acquisition state")
        .allowedStates("Ok.Ready")
        .commit()
        ,
        SLOT_ELEMENT(expected).key("trigger")
        .displayedName("Trigger")
        .description("Sends a software trigger to the camera")
        .allowedStates("Ok.Acquisition")
        .commit()
        ,
        SLOT_ELEMENT(expected).key("stop")
        .displayedName("Stop")
        .description("Instructs camera to stop current acquisition")
        .allowedStates("Ok.Acquisition")
        .commit()
        ,
        SLOT_ELEMENT(expected).key("reset")
        .displayedName("Reset")
        .description("Resets the camera in case of an error")
        .allowedStates("Error")
        .commit()
        )
    
    def __init__(self, configuration):
        super(CameraFsm, self).__init__(configuration)
        
        #**************************************************************
        #*                        Events                              *
        #**************************************************************
        KARABO_FSM_EVENT(self, 'ErrorFoundEvent', 'errorFound')
        KARABO_FSM_EVENT(self, 'ResetEvent',      'reset')
        KARABO_FSM_EVENT(self, 'AcquireEvent',    'acquire')
        KARABO_FSM_EVENT(self, 'StopEvent',       'stop')
        KARABO_FSM_EVENT(self, 'TriggerEvent',    'trigger')

        #**************************************************************
        #*                        States                              *
        #**************************************************************
        KARABO_FSM_STATE('Error', self.errorStateOnEntry, self.errorStateOnExit)
        KARABO_FSM_STATE('Initialization', self.initializationStateOnEntry, self.initializationStateOnExit)
        KARABO_FSM_STATE('Acquisition', self.acquisitionStateOnEntry, self.acquisitionStateOnExit)
        KARABO_FSM_STATE('Ready', self.readyStateOnEntry, self.readyStateOnExit)
        
        #**************************************************************
        #*                    Transition Actions                      *
        #**************************************************************
        #KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)    
        KARABO_FSM_ACTION('ErrorFoundAction', self.errorFoundAction, str, str)
        KARABO_FSM_ACTION('ResetAction', self.resetAction)
        KARABO_FSM_ACTION('AcquireAction', self.acquireAction)
        KARABO_FSM_ACTION('StopAction',  self.stopAction)
        KARABO_FSM_ACTION('TriggerAction',  self.triggerAction)
        
        #**************************************************************
        #*                       Ok State Machine                     *
        #**************************************************************
        okStt=[
        # Source-State   Event           Target-State   Action           Guard
        ('Ready',       'AcquireEvent', 'Acquisition', 'AcquireAction', 'none'),
        ('Acquisition', 'StopEvent',    'Ready',       'StopAction',    'none'),
        ('Acquisition', 'TriggerEvent', 'none',        'TriggerAction', 'none')
        ]
        #                        Name  Transition-Table  Initial-State
        KARABO_FSM_STATE_MACHINE('Ok', okStt, 'Ready')
        
        #**************************************************************
        #*                       Top Machine                          *
        #**************************************************************
        cameraStt=[
        # Source-State      Event        Target-State   Action              Guard
        ('Initialization', 'none',            'Ok',    'none',             'none'),
        ('Ok',             'ErrorFoundEvent', 'Error', 'ErrorFoundAction', 'none'),
        ('Error',          'ResetEvent',      'Ok',    'none',             'none')
        ]
        #                         Name      Transition-Table  Initial-State
        KARABO_FSM_STATE_MACHINE('CameraMachine', cameraStt, 'Initialization')
        self.fsm = KARABO_FSM_CREATE_MACHINE('CameraMachine')
        
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
        
    def acquisitionStateOnEntry(self):
        '''Actions executed on entry to 'Acquisition' state
        '''
        
    def acquisitionStateOnExit(self):
        '''Actions executed on exit from 'Acquisition' state
        '''
        
    def readyStateOnEntry(self):
        '''Actions executed on entry to 'Ready' state
        '''
        
    def readyStateOnExit(self):
        '''Actions executed on exit from 'Ready' state
        '''
        
    def acquireAction(self):
        '''Actions executed at 'acquire' event'''
        
    def stopAction(self):
        '''Actions executed at 'stop' event'''
        
    def triggerAction(self):
        '''Actions executed at 'trigger' event'''

    def resetAction(self):
        print("Reset action executed")
        
