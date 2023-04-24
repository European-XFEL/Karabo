# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

import unittest

from karabo.bound import Hash, Logger, OkErrorFsm, State


class OkErrorUser(OkErrorFsm):
    config = Hash("priority", "DEBUG")
    Logger.configure(config)
    log = Logger.getCategory()

    def __init__(self, configuration):
        super(OkErrorUser, self).__init__(configuration)

    # The following 2 methods should be always defined
    def noStateTransition(self):
        self.log.DEBUG("-- OkErrorUser.noStateTransition")

    def updateState(self, currentState):
        self.log.DEBUG(
            "-- OkErrorUser.updateState to '{}'".format(currentState))

    def errorStateOnEntry(self):
        self.log.DEBUG("-- OkErrorUser.errorStateOnEntry")

    def errorStateOnExit(self):
        self.log.DEBUG("-- OkErrorUser.errorStateOnExit")

    def resetAction(self):
        self.log.DEBUG("-- OkErrorUser.resetAction")

    def okStateOnEntry(self):
        self.log.DEBUG("-- OkErrorUser.okStateOnEntry")

    def okStateOnExit(self):
        self.log.DEBUG("-- OkErrorUser.okStateOnExit")


class Ok_error_fsm_TestCase(unittest.TestCase):
    def setUp(self):
        self.okerr = OkErrorUser(None)

    def tearDown(self):
        self.okerr = None

    def test_ok_error_fsm_(self):
        # assert x != y;
        # self.assertEqual(x, y, "Msg");
        # self.fail("TODO: Write test")
        fsm = self.okerr.fsm
        fsm.start()
        self.assertIs(fsm.get_state(), State.NORMAL)
        self.okerr.log.DEBUG("*** State 'Ok' reached")
        self.okerr.errorFound("user error message", "detailed error message")
        self.assertEqual(fsm.get_state(), State.ERROR, "Assert failed")
        self.okerr.log.DEBUG("*** State 'Error' reached")
        self.okerr.reset()
        self.okerr.log.DEBUG("*** State 'Ok' reached")
        self.assertEqual(fsm.get_state(), State.NORMAL, "Assert failed")


if __name__ == '__main__':
    unittest.main()
