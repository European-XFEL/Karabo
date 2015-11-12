# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

import unittest

from karabo.ok_error_fsm import OkErrorFsm


class OkErrorUser(OkErrorFsm):
    def __init__(self, configuration):
        super(OkErrorUser, self).__init__(configuration)

    # The following 2 methods should be always defined 
    def noStateTransition(self):
        print("-- OkErrorUser.noStateTransition")
        
    def updateState(self, currentState):
        print("-- OkErrorUser.updateState to '{}'".format(currentState))
    
    def errorStateOnEntry(self):
        print("-- OkErrorUser.errorStateOnEntry")
        
    def errorStateOnExit(self):
        print("-- OkErrorUser.errorStateOnExit")

    def resetAction(self):
        print("-- OkErrorUser.resetAction")
        
    def okStateOnEntry(self):
        print("-- OkErrorUser.okStateOnEntry")
        
    def okStateOnExit(self):
        print("-- OkErrorUser.okStateOnExit")
        
    
class  Ok_error_fsm_TestCase(unittest.TestCase):
    def setUp(self):
        self.okerr = OkErrorUser(None)
    
    def tearDown(self):
        self.okerr = None

    def test_ok_error_fsm_(self):
        #assert x != y;
        #self.assertEqual(x, y, "Msg");
        #self.fail("TODO: Write test")
        fsm = self.okerr.fsm
        fsm.start()
        self.assertEqual(fsm.get_state(), "Ok", "Assert failed")
        print ("*** State 'Ok' reached")
        self.okerr.errorFound("user error message", "detailed error message")
        self.assertEqual(fsm.get_state(), "Error", "Assert failed")
        print ("*** State 'Error' reached")
        self.okerr.reset()
        print ("*** State 'Ok' reached")
        self.assertEqual(fsm.get_state(), "Ok", "Assert failed")
        

if __name__ == '__main__':
    unittest.main()

