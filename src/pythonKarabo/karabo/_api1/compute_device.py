# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$Sep 10, 2013 4:17:06 PM$"

from abc import abstractmethod
import threading

import karabo._api1.base_fsm as base
from karathon import BOOL_ELEMENT, INT32_ELEMENT, SLOT_ELEMENT
from .decorators import KARABO_CLASSINFO
from .device import PythonDevice
from .fsm import (
    KARABO_FSM_STATE, KARABO_FSM_STATE_E, KARABO_FSM_STATE_EE,
    KARABO_FSM_INTERRUPT_STATE, KARABO_FSM_EVENT0, KARABO_FSM_EVENT2,
    KARABO_FSM_ACTION0, KARABO_FSM_ACTION2, KARABO_FSM_GUARD0,
    KARABO_FSM_CREATE_MACHINE, KARABO_FSM_STATE_MACHINE
)


@KARABO_CLASSINFO("ComputeFsm", "1.0")
class ComputeFsm(base.BaseFsm):
    
    @staticmethod
    def expectedParameters(expected):
        (
        SLOT_ELEMENT(expected).key("start")
                .displayedName("Compute")
                .description("Starts computing if data is available")
                .allowedStates("Ok:Ready")
                .commit()
                ,
        SLOT_ELEMENT(expected).key("pause")
                .displayedName("Pause")
                .description("Will finish current computation and pause")
                .allowedStates("Ok:Ready")
                .commit()
                ,
        SLOT_ELEMENT(expected).key("abort")
                .displayedName("Abort")
                .description("Abort contribution to this run, fully disconnect")
                .allowedStates("Ok:Ready Ok:Computing Ok:WaitingIO Ok:Paused")
                .commit()
                ,
        SLOT_ELEMENT(expected).key("endOfStream")
                .displayedName("End-Of-Stream")
                .description("Completely reset this device")
                .allowedStates("Ok:Ready")
                .commit()
                ,
        SLOT_ELEMENT(expected).key("reset")
                .displayedName("Reset")
                .description("Completely reset this device")
                .allowedStates("Error:Ready Error:Computing Error:WaitingIO Ok:Finished Ok:Aborted")
                .commit()
                ,
        )
        
    def __init__(self, configuration):
        super(ComputeFsm, self).__init__(configuration)
        
        #**************************************************************
        #*                        Events                              *
        #**************************************************************
        KARABO_FSM_EVENT2(self, 'ErrorFoundEvent',       'errorFound')
        KARABO_FSM_EVENT0(self, 'ResetEvent',            'reset')
        KARABO_FSM_EVENT0(self, "StartEvent",            'start')
        KARABO_FSM_EVENT0(self, "EndOfStreamEvent",      'endOfStream')
        KARABO_FSM_EVENT0(self, 'PauseEvent',            'pause')
        KARABO_FSM_EVENT0(self, 'AbortEvent',            'abort')
        KARABO_FSM_EVENT0(self, 'ComputeFinishedEvent',  'computeFinished')
        KARABO_FSM_EVENT0(self, 'UpdatedIOEvent',        'updatedIO')

        #**************************************************************
        #*                        States                              *
        #**************************************************************
        KARABO_FSM_STATE   ('Ok')
        KARABO_FSM_INTERRUPT_STATE('Error', 'ResetEvent')
        KARABO_FSM_STATE   ('ConnectingIO')
        KARABO_FSM_STATE_E ('Ready',     self.readyStateOnEntry)
        KARABO_FSM_STATE_EE('Computing', self.computingStateOnEntry, self.computingStateOnExit)
        KARABO_FSM_STATE_EE('WaitingIO', self.waitingIOOnEntry,      self.waitingIOOnExit)
        KARABO_FSM_STATE   ('Paused')
        KARABO_FSM_STATE_EE('Finished',  self.finishedOnEntry,       self.finishedOnExit)
        KARABO_FSM_STATE_E ('Aborted',   self.abortedOnEntry)

        #**************************************************************
        #*                    Transition Actions                      *
        #**************************************************************
        #KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)  # <- this is defined in BaseFsm
        KARABO_FSM_ACTION2('ErrorFoundAction',    self.errorFoundAction, str, str)
        KARABO_FSM_ACTION0('ConnectAction',       self.connectAction)
        KARABO_FSM_ACTION0('EndOfStreamAction',   self.endOfStreamAction)
        KARABO_FSM_ACTION0('NextIterationAction', self.onNextIteration)

        #**************************************************************
        #*                    Guards                                  *
        #**************************************************************
        KARABO_FSM_GUARD0('CanCompute', self.canCompute)
        KARABO_FSM_GUARD0('AbortGuard', self.registerAbort)
        KARABO_FSM_GUARD0('PauseGuard', self.registerPause)
        
        #**************************************************************
        #*                    State Machine                           *
        #**************************************************************
        onSTT = [
        # Source-State        Event              Target-State    Action             Guard
            ('ConnectingIO', 'none',             'Ready',     'ConnectAction',      'none'),
            ('Ready',        'StartEvent',       'Computing', 'none',               'CanCompute'),
            ('Ready',        'PauseEvent',       'Paused',    'none',               'none'),
            ('Ready',        'AbortEvent',       'Aborted',   'none',               'none'),
            ('Ready',        'EndOfStreamEvent', 'Finished',  'EndOfStreamAction',  'none'),
            ('Paused',       'ResetEvent',       'Ready',     'none',               'CanCompute'),
            ('Computing','ComputeFinishedEvent', 'WaitingIO', 'none',               'none'),
            ('Computing',    'AbortEvent',       'Aborted',   'none',               'AbortGuard'),
            ('Computing',    'PauseEvent',       'Paused',    'none',               'PauseGuard'),
            ('WaitingIO',    'UpdatedIOEvent',   'Ready',     'none',               'none'),
            ('Aborted',      'ResetEvent',       'Ready',     'none',               'none'),
            ('Finished',     'ResetEvent',       'Ready',     'none',               'none'),
            ('Finished',     'StartEvent',       'Computing', 'NextIterationAction','none'),
            ('Ok',           'ErrorFoundEvent',  'Error',     'ErrorFoundAction',   'none'),
            ('Error',        'ResetEvent',       'Ok',        'none',               'none')
        ]

        #                        Name     Transition-Table   Initial-State
        KARABO_FSM_STATE_MACHINE('StateMachine', onSTT, ('Ok', 'ConnectingIO'))
        self.fsm = KARABO_FSM_CREATE_MACHINE('StateMachine')
    
    def getFsm(self):
        return self.fsm
    
    def initFsmSlots(self, sigslot):
        sigslot.registerSlot(self.errorFound)
        sigslot.registerSlot(self.reset)
        sigslot.registerSlot(self.start)
        sigslot.registerSlot(self.endOfStream)
        sigslot.registerSlot(self.pause)
        sigslot.registerSlot(self.abort)
        sigslot.registerSlot(self.computeFinished)
        sigslot.registerSlot(self.updatedIO)
        
    ########################################################
    #  Guards, Transition Actions, State Machine hooks...  #
    ########################################################
    
    @abstractmethod
    def readyStateOnEntry(self):
        pass
    
    @abstractmethod
    def computingStateOnEntry(self):
        pass
    
    @abstractmethod
    def computingStateOnExit(self):
        pass
    
    @abstractmethod
    def waitingIOOnEntry(self):
        pass
    
    @abstractmethod
    def waitingIOOnExit(self):
        pass
    
    @abstractmethod
    def finishedOnEntry(self):
        pass
    
    @abstractmethod
    def finishedOnExit(self):
        pass
    
    @abstractmethod
    def abortedOnEntry(self):
        pass
    
    @abstractmethod
    def errorFoundAction(self, s1, s2):
        pass
    
    @abstractmethod
    def connectAction(self):
        pass
    
    @abstractmethod
    def endOfStreamAction(self):
        pass
    
    @abstractmethod
    def onNextIteration(self):
        pass
    
    @abstractmethod
    def canCompute(self):
        return True
    
    @abstractmethod
    def registerAbort(self):
        return True
    
    @abstractmethod
    def registerPause(self):
        return True


@KARABO_CLASSINFO("PythonComputeDevice", "1.0")
class PythonComputeDevice(PythonDevice, ComputeFsm):
    
    def __init__(self, configuration):
        super(PythonComputeDevice, self).__init__(configuration)
        self.isAborted = False
        self.isEndOfStream = False
        self.deviceIsDead = False
        self.nEndOfStreams = 0
        self.iterationCount = 0
        self.computeLock = threading.Lock()
        self.computeLock.acquire(True)
        self.computeThread = threading.Thread(target=self.doCompute)
        self.computeThread.start()
        self.waitingIOLock = threading.Lock()
        self.waitingIOLock.acquire(True)
        self.waitingIOThread = threading.Thread(target=self.doWait)
        self.waitingIOThread.start()

    def __del__(self):
        ''' PythonComputeDevice destructor '''
        self.deviceIsDead = True
        self.computeLock.release()
        self.computeThread.join()
        self.waitingIOLock.release()
        self.waitingIOThread.join()
        super(PythonComputeDevice, self).__del__()
        
    @staticmethod
    def expectedParameters(expected):
        (
        BOOL_ELEMENT(expected).key("autoCompute")
                .displayedName("Auto Compute")
                .description("Trigger computation automatically once data is available")
                .reconfigurable()
                .assignmentOptional().defaultValue(True)
                .commit()
                ,
        BOOL_ELEMENT(expected).key("autoEndOfStream")
                .displayedName("Auto end-of-stream")
                .description("If true, automatically forwards the end-of-stream signal to all connected (downstream) devices")
                .reconfigurable()
                .assignmentOptional().defaultValue(True)
                .commit()
                ,
        BOOL_ELEMENT(expected).key("autoIterate")
                .displayedName("Auto iterate")
                .description("If true, automatically iterates cyclic workflows")
                .reconfigurable()
                .assignmentOptional().defaultValue(True)
                .commit()
                ,
        INT32_ELEMENT(expected).key("iteration")
                .displayedName("Iteration")
                .description("The current iteration")
                .readOnly()
                .initialValue(0)
                .commit()
                ,
        )
        
    def KARABO_INPUT_CHANNEL(self, type, name, configuration):
        if type.__name__ == "InputHash":
            return self._ss.createInputChannelHash(name, configuration, self._onInputAvailable, self._onEndOfStream)
    
    def KARABO_OUTPUT_CHANNEL(self, type, name, configuration):
        if type.__name__ == "OutputHash":
            return self._ss.createOutputChannelHash(name, configuration)
        
    @abstractmethod    
    def compute(self):
        ''' Put your specific algorithms here '''

    def _onInputAvailable(self, input):
        if self.get("state") == "Ok.Finished" and not self.get("autoIterate"):
            pass
        else:
            self.start()
    
    def _onEndOfStream(self):
        self.nEndOfStreams = self.nEndOfStreams + 1
        if self.nEndOfStreams >= len(self._ss.getInputChannels()):
            self.nEndOfStreams = 0
            self.isEndOfStream = True
            if self.get("autoEndOfStream"): 
                self.endOfStream();
    
    def endOfStreamAction(self):
        self.onEndOfStream()
        outputChannels = self._ss.getOutputChannels()
        for c in outputChannels:
            #channel.signalEndOfStream()
            c.data().signalEndOfStream()
            
    
    def onEndOfStream(self):
        ''' Override this function for specializing the endOfStream behavior '''
        
    def  update(self):
        ''' Override this function for specializing the update behaviors of your IO channels '''
        inputChannels = self._ss.getInputChannels()
        outputChannels = self._ss.getOutputChannels()
        for c in inputChannels:
            c.data().update()
        for c in outputChannels:
            c.data().update()
        
    def isAborted(self):
        return self.isAborted;
    
    def connectAction(self):
        return self._ss.connectInputChannels()
    
    def readyStateOnEntry(self):
        if self.isEndOfStream and self.get("autoEndOfStream"):
            self.endOfStream()
        elif self.isAborted:
            self.abort()
        elif len(self._ss.getInputChannels()) > 0 and self.get("autoCompute"):
            self.start()
            
    def canCompute(self):
        inputChannels = self._ss.getInputChannels()
        for c in inputChannels:
            if not c.data().canCompute():
                return False
        return True
    
    def computingStateOnEntry(self):
        self.computeLock.release()
        
    def doCompute(self):
        while True:
            self.computeLock.acquire(True)
            if self.deviceIsDead:
                return
            self.compute()
            self.computeLock.release()
            if not self.isAborted:
                self.computeFinished()
        
    def computingStateOnExit(self):
        self.computeLock.acquire(True)
        
    def registerAbort(self):
        self.isAborted = True;
        return True
    
    def registerPause(self):
        return True
    
    def waitingIOOnEntry(self):
        self.waitingIOLock.release()

    def doWait(self):
        while True:
            self.waitingIOLock.acquire(True)
            if self.deviceIsDead:
                return
            try:
                self.update()
            except Exception as e:
                self.waitingIOLock.release()
                self.errorFound("Exception caught", str(e))
                return
            self.waitingIOLock.release()
            self.updatedIO()
            
    def waitingIOOnExit(self):
        self.waitingIOLock.acquire(True)
        
    def finishedOnEntry(self):
        self.iterationCount = self.iterationCount + 1
        self.isEndOfStream = False
        
    def finishedOnExit(self):
        self.set("iteration", self.iterationCount)
        
    def abortedOnEntry(self):
        self.isAborted = False
        
