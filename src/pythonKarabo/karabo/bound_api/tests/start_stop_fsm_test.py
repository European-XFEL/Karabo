# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

import unittest

from karabo.bound import Hash, Logger, StartStopFsm, State


class StartStopUser(StartStopFsm):
    config = Hash("priority", "DEBUG")
    Logger.configure(config)
    log = Logger.getCategory()

    def __init__(self, configuration):
        super().__init__(configuration)

    # The following 2 methods should be always defined
    def noStateTransition(self):
        self.log.DEBUG("-- StartStopUser.noStateTransition")

    def updateState(self, currentState):
        self.log.DEBUG(
            f"-- StartStopUser.updateState to '{currentState}'")

    def errorStateOnEntry(self):
        self.log.DEBUG("-- StartStopUser.errorStateOnEntry")

    def errorStateOnExit(self):
        self.log.DEBUG("-- StartStopUser.errorStateOnExit")

    def startedStateOnEntry(self):
        self.log.DEBUG("-- StartStopUser.startedStateOnEntry")

    def startedStateOnExit(self):
        self.log.DEBUG("-- StartStopUser.startedStateOnExit")

    def stoppedStateOnEntry(self):
        self.log.DEBUG("-- StartStopUser.stoppedStateOnEntry")

    def stoppedStateOnExit(self):
        self.log.DEBUG("-- StartStopUser.stoppedStateOnExit")

    def errorFoundAction(self, m1, m2):
        self.log.DEBUG("-- StartStopUser.errorFoundAction")

    def startAction(self):
        self.log.DEBUG("-- StartStopUser.startAction")

    def stopAction(self):
        self.log.DEBUG("-- StartStopUser.stopAction")

    def resetAction(self):
        self.log.DEBUG("-- StartStopUser.resetAction")


class Start_stop_fsm_TestCase(unittest.TestCase):
    def setUp(self):
        self.startstop = StartStopUser(None)

    def tearDown(self):
        self.startstop = None

    def test_start_stop_fsm_(self):
        fsm = self.startstop.fsm
        fsm.start()
        self.assertIs(fsm.get_state(), State.STOPPED)
        self.startstop.log.DEBUG("*** State 'STOPPED' reached")
        self.startstop.start()
        self.assertIs(fsm.get_state(), State.STARTED)
        self.startstop.log.DEBUG("*** State 'STARTED' reached")
        self.startstop.stop()
        self.assertIs(fsm.get_state(), State.STOPPED)
        self.startstop.log.DEBUG("*** State 'STOPPED' reached")
        self.startstop.errorFound("user error message",
                                  "detailed error message")
        self.assertIs(fsm.get_state(), State.ERROR)
        self.startstop.log.DEBUG("*** State 'ERROR' reached")
        self.startstop.reset()
        self.assertIs(fsm.get_state(), State.STOPPED)
        self.startstop.log.DEBUG("*** State 'STOPPED' reached")


if __name__ == '__main__':
    unittest.main()
