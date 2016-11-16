# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

import unittest

from karabo.bound import CameraInterface, Hash, Logger, State, Schema

class CameraUser(CameraInterface):
    config = Hash("priority", "DEBUG")
    Logger.configure(config)
    log = Logger.getCategory()

    def __init__(self, configuration):
        super(CameraUser, self).__init__(configuration)

    def initialize(self):
        self.log.DEBUG("-- CameraUser.initialize")

    def acquire(self):
        self.log.DEBUG("-- CameraUser.acquire")

    def trigger(self):
        self.log.DEBUG("-- CameraUser.trigger")

    def stop(self):
        self.log.DEBUG("-- CameraUser.stop")

    def resetHardware(self):
        self.log.DEBUG("-- CameraUser.resetHardware")

class  Camera_interface_TestCase(unittest.TestCase):
    def setUp(self):
        self.camera = CameraUser(None)
    
    def tearDown(self):
        self.camera = None

    def test_camera_interface_(self):
        expected = Schema()
        self.camera.log.DEBUG("*** Calling expectedParameters(...)")
        self.camera.expectedParameters(expected)

if __name__ == '__main__':
    unittest.main()
