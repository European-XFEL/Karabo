# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov <serguei.essenov at xfel.eu>"
__date__ ="$May 10, 2013 2:17:13 PM$"

import threading
from abc import ABCMeta, abstractmethod
from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS

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
        with self.processEventLock:
            fsm = self.getFsm()
            if fsm is not None:
                self.onStateUpdate("Changing...")
                try:
                    fsm.process_event(event)
                except Exception, e:
                    self.errorFound("Exception while processing event '{}'".format(event.__class__.__name__), str(e))
                    return
                self.onStateUpdate(fsm.get_state())
    
