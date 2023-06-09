# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
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
        super().__init__(configuration)

    # The following 2 methods should be always defined
    def noStateTransition(self):
        self.log.DEBUG("-- OkErrorUser.noStateTransition")

    def updateState(self, currentState):
        self.log.DEBUG(
            f"-- OkErrorUser.updateState to '{currentState}'")

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
