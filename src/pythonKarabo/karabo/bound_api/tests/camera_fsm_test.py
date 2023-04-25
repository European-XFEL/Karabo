# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

import unittest

from karabo.bound import CameraFsm, Hash, Logger, State


class CameraUser(CameraFsm):
    config = Hash("priority", "DEBUG")
    Logger.configure(config)
    log = Logger.getCategory()

    def __init__(self, configuration):
        super(CameraUser, self).__init__(configuration)

    # The following 2 methods should be always defined
    def noStateTransition(self):
        self.log.DEBUG("-- CameraUser.noStateTransition")

    def updateState(self, currentState):
        self.log.DEBUG("-- CameraUser.updateState "
                       "to '{}'".format(currentState))

    def initializationStateOnEntry(self):
        self.log.DEBUG("-- CameraUser.initializationStateOnEntry")

    def initializationStateOnExit(self):
        self.log.DEBUG("-- CameraUser.initializationStateOnExit")

    def unknownStateOnEntry(self):
        self.log.DEBUG("-- CameraUser.unknownStateOnEntry")

    def unknownStateOnExit(self):
        self.log.DEBUG("-- CameraUser.unknownStateOnExit")

    def errorStateOnEntry(self):
        self.log.DEBUG("-- CameraUser.errorStateOnEntry")

    def errorStateOnExit(self):
        self.log.DEBUG("-- CameraUser.errorStateOnExit")

    def acquisitionStateOnEntry(self):
        self.log.DEBUG("-- CameraUser.acquisitionStateOnEntry")

    def acquisitionStateOnExit(self):
        self.log.DEBUG("-- CameraUser.acquisitionStateOnExit")

    def readyStateOnEntry(self):
        self.log.DEBUG("-- CameraUser.readyStateOnEntry")

    def readyStateOnExit(self):
        self.log.DEBUG("-- CameraUser.readyStateOnExit")

    def errorFoundAction(self, m1, m2):
        self.log.DEBUG("-- CameraUser.errorFoundAction")

    def acquireAction(self):
        self.log.DEBUG("-- CameraUser.acquireAction")

    def stopAction(self):
        self.log.DEBUG("-- CameraUser.stopAction")

    def triggerAction(self):
        self.log.DEBUG("-- CameraUser.stopAction")

    def resetAction(self):
        self.log.DEBUG("-- CameraUser.resetAction")

    def connectGuard(self):
        return True


class Camera_fsm_TestCase(unittest.TestCase):
    def setUp(self):
        self.camera = CameraUser(None)

    def tearDown(self):
        self.camera = None

    def test_camera_fsm_(self):
        fsm = self.camera.fsm
        fsm.start()
        self.assertIs(fsm.get_state(), State.UNKNOWN)
        self.camera.log.DEBUG("*** State 'UNKNOWN' reached")
        self.camera.connectCamera()
        self.assertIs(fsm.get_state(), State.ON)
        self.camera.log.DEBUG("*** State 'ON' reached")
        self.camera.acquire()
        self.assertIs(fsm.get_state(), State.ACQUIRING)
        self.camera.log.DEBUG("*** State 'ACQUIRING' reached")
        self.camera.trigger()
        self.assertIs(fsm.get_state(), State.ACQUIRING)
        self.camera.log.DEBUG("*** Trigger sent. Remain in state 'ACQUIRING'")
        self.camera.stop()
        self.assertIs(fsm.get_state(), State.ON)
        self.camera.log.DEBUG("*** State 'ON' reached")
        self.camera.errorFound("user error message", "detailed error message")
        self.assertIs(fsm.get_state(), State.ERROR)
        self.camera.log.DEBUG("*** State 'ERROR' reached")
        self.camera.reset()
        self.assertIs(fsm.get_state(), State.ON)
        self.camera.log.DEBUG("*** State 'ON' reached")
        self.camera.disconnectCamera()
        self.assertIs(fsm.get_state(), State.UNKNOWN)
        self.camera.log.DEBUG("*** State 'UNKNOWN' reached")


if __name__ == '__main__':
    unittest.main()
