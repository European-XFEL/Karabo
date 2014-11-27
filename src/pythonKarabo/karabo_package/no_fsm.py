# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov serguei.essenov@xfel.eu"
__date__ ="$Nov 26, 2014 3:18:24 PM$"

from karabo.decorators import KARABO_CLASSINFO
import karabo.base_fsm as base

@KARABO_CLASSINFO("NoFsm", "1.2 1.3")
class NoFsm(base.BaseFsm):
    
    @staticmethod
    def expectedParameters(expected):
        pass

    def __init__(self, configuration):
        super(NoFsm, self).__init__(configuration)
        self.func = None
    
    def getFsm(self):
        return None
    
    def initFsmSlots(self, sigslot):
        pass
    
    def startFsm(self):
        """Start state machine"""
        self.updateState("Changing...")
        if self.func is None:
            raise RuntimeError("No initial function defined. Please call 'initialFunc' method in the device constructor")
        self.func()    # call initial function registered in the device constructor
        
    def processEvent(self, event):
        pass

    def initialFunc(self, func):
        self.func = func