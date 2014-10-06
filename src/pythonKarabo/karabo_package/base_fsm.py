# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$May 10, 2013 2:17:13 PM$"

import threading
from abc import ABCMeta, abstractmethod
from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karabo.fsm import KARABO_FSM_NO_TRANSITION_ACTION

@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("BaseFsm", "1.0")
class BaseFsm(object):
    
    @staticmethod
    def expectedParameters(expected):
        pass
    
    def __init__(self, configuration):
        super(BaseFsm, self).__init__()
        self.fsm = None
        self.processEventLock = threading.RLock()
        KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)
    
    def getFsm(self):
        return self.fsm
    
    def initFsmSlots(self, sigslot):
        pass
    
    @abstractmethod
    def exceptionFound(self, shortMessage, detailedMessage):
        pass

    # TODO Deprecate
    @abstractmethod
    def onStateUpdate(self, currentState):
        pass   

    @abstractmethod
    def updateState(self, currentState):
        pass
    
    def noStateTransition(self):
        print("*** No transition exists for the last event ***")
    
    def startFsm(self):
        """Start state machine"""
        fsm = self.getFsm()
        if fsm is not None:
            try:
                fsm.start()
            except Exception as e:
                self.exceptionFound("Exception while processing event '{}'".format("Start state machine"), str(e))
                return
            # this is for compatibility with GUI: strip square brackets from state name in case of state machine with regions
            state = fsm.get_state()
            if state[0] == '[' and state[len(state)-1] == ']':
                state = state[1:len(state)-1]
            self.updateState(state)
    
    def processEvent(self, event):
        """Process input event, i.e. drive state machine to the next state."""
        with self.processEventLock:
            fsm = self.getFsm()
            if fsm is not None:
                self.updateState("Changing...")
                try:
                    fsm.process_event(event)
                except Exception as e:
                    self.errorFound("Exception while processing event '{}'".format(event.__class__.__name__), str(e))
                    return
                state = fsm.get_state()
                # this is for compatibility with GUI: strip square brackets from state name in case of state machine with regions
                if state[0] == '[' and state[len(state)-1] == ']':
                    state = state[1:len(state)-1]
                self.updateState(state)
    
