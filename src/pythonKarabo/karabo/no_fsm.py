# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov serguei.essenov@xfel.eu"
__date__ ="$Nov 26, 2014 3:18:24 PM$"


from karabo.decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from karabo.worker import Worker

@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("NoFsm", "1.3")
class NoFsm(object):
    
    @staticmethod
    def expectedParameters(expected):
        pass

    def __init__(self, configuration):
        super(NoFsm, self).__init__()
        self.func = None
    
    def startFsm(self):
        """Start state machine"""
        #self.updateState("Changing...")
        if self.func is None:
            raise RuntimeError("No initial function defined. Please call 'initialFunc' method in the device constructor")
        self.func()    # call initial function registered in the device constructor
        
    def registerInitialFunction(self, func):
        self.func = func
        
    def stopFsm(self): pass
    
    def errorFound(self, userFriendly, detail):
        print("*** ERROR *** : {} -- {}".format(userFriendly, detail))
