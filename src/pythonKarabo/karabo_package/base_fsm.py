# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$May 10, 2013 2:17:13 PM$"

from abc import ABCMeta, abstractmethod
from asyncio import async, coroutine, get_event_loop, Lock
from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karabo.fsm import KARABO_FSM_NO_TRANSITION_ACTION
from karabo.schema import Configurable

@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("BaseFsm", "1.0")
class BaseFsm(Configurable):
    def __init__(self, configuration):
        super(BaseFsm, self).__init__(configuration)
        self.fsm = None
        self.processEventLock = Lock()
        KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)
    
    def getFsm(self):
        return self.fsm
    
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


    def run(self):
        async(self.startFsm())
        super().run()


    @coroutine
    def startFsm(self):
        """Start state machine"""
        fsm = self.getFsm()
        if fsm is not None:
            yield from get_event_loop().run_in_executor(None, fsm.start)
            # this is for compatibility with GUI: strip square brackets from state name in case of state machine with regions
            state = fsm.get_state()
            if state[0] == '[' and state[len(state)-1] == ']':
                state = state[1:len(state)-1]
            self.updateState(state)


    @coroutine
    def processEvent(self, event):
        """Process input event, i.e. drive state machine to the next state."""
        with (yield from self.processEventLock):
            fsm = self.getFsm()
            if fsm is not None:
                self.updateState("Changing...")
                yield from get_event_loop().run_in_executor(
                        None, fsm.process_event, event)
                state = fsm.get_state()
                # this is for compatibility with GUI: strip square brackets from state name in case of state machine with regions
                if state[0] == '[' and state[len(state)-1] == ']':
                    state = state[1:len(state)-1]
                self.updateState(state)
    
