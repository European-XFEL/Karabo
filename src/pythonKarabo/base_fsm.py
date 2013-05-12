# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$May 10, 2013 2:17:13 PM$"

from abc import ABCMeta, abstractmethod
from karabo_decorators import *

@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("BaseFsm", "1.0")
class BaseFsm(object):
    
    @staticmethod
    def expectedParameters(expected):
        pass
    
    def __init__(self, *args, **kwargs):
        super(BaseFsm, self).__init__()
        self.fsm = None
    
    def getFsm(self):
        return self.fsm
    
    def initFsmSlots(self, sigslot):
        pass
    
    @abstractmethod
    def errorFound(self, shortMessage, detailedMessage):
        pass
    
    @abstractmethod
    def errorFoundAction(self, shortMessage, detailedMessage):
        pass
    
    @abstractmethod
    def onStateUpdate(self, currentState):
        pass
    
    @abstractmethod
    def noStateTransition(self):
        pass
    
    def startFsm(self):
        """Start state machine"""
        fsm = self.getFsm()
        if fsm is not None:
            fsm.start()
            self.onStateUpdate(fsm.get_state())
    
    def processEvent(self, event):
        """Process input event, i.e. drive state machine to the next state."""
        fsm = self.getFsm()
        if fsm is not None:
            self.onStateUpdate("Changing...")
            fsm.process_event(event)
            self.onStateUpdate(fsm.get_state())
    
