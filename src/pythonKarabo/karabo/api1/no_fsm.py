# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

__author__="Sergey Esenov serguei.essenov@xfel.eu"
__date__ ="$Nov 26, 2014 3:18:24 PM$"


from .decorators import KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS
from .worker import Worker


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO("NoFsm", "1.3")
class NoFsm(object):
    
    @staticmethod
    def expectedParameters(expected):
        pass

    def __init__(self, configuration):
        super(NoFsm, self).__init__()
        self.func = []
    
    def startFsm(self):
        """Start state machine"""
        #self.updateState("Changing...")
        if len(self.func) == 0:
            raise RuntimeError("No initial function defined. Please call 'registerInitialFunction' method in the device constructor")
        # call initial function registered in the device constructor on registration order
        for f in self.func: f()
        
    def registerInitialFunction(self, func):
        self.func.append(func)
        
    def stopFsm(self): pass
    
    def errorFound(self, userFriendly, detail):
        print("*** ERROR *** : {} -- {}".format(userFriendly, detail))
