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
    
    @abstractmethod
    def setupFsm(self):
        self.fsm = None
    
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
        if self.fsm is not None:
            self.fsm.start()
            self.onStateUpdate(self.fsm.get_state())
