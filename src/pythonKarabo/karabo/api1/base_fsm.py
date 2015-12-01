# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$May 10, 2013 2:17:13 PM$"

import threading

from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from .fsm import KARABO_FSM_NO_TRANSITION_ACTION
from .no_fsm import NoFsm


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("BaseFsm", "1.0")
class BaseFsm(NoFsm):
    
    @staticmethod
    def expectedParameters(expected): pass
    
    def __init__(self, configuration):
        super(BaseFsm, self).__init__(configuration)
        self.fsm = None
        self.processEventLock = threading.RLock()
        KARABO_FSM_NO_TRANSITION_ACTION(self.noStateTransition)
    
    def getFsm(self): return self.fsm
    
    def startFsm(self):
        """Start state machine"""
        fsm = self.getFsm()
        if fsm is not None:
            try:
                fsm.start()
            except Exception as e:
                raise RuntimeError("startFsm -- Exception: {0}".format(str(e)))
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
                #self.updateState("Changing...")
                try:
                    fsm.process_event(event)
                except Exception as e:
                    raise RuntimeError("processEvent: event = '{0}' -- Exception: {1}".format(event.__class__.__name__, str(e)))
                state = fsm.get_state()
                # this is for compatibility with GUI: strip square brackets from state name in case of state machine with regions
                if state[0] == '[' and state[len(state)-1] == ']':
                    state = state[1:len(state)-1]
                self.updateState(state)
    
