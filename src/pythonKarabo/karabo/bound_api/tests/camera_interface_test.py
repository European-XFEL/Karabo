# To change this license header, choose License Headers in Project Properties.
# To change this template file, choose Tools | Templates
# and open the template in the editor.

import unittest

from karabo.bound import CameraInterface, Schema, State, STATE_ELEMENT


class CameraUser(CameraInterface):
    def __init__(self, configuration):
        super(CameraUser, self).__init__(configuration)

    def initialize(self):
        pass

    def connectCamera(self):
        pass

    def acquire(self):
        pass

    def trigger(self):
        pass

    def stop(self):
        pass

    def resetHardware(self):
        pass


class Camera_interface_TestCase(unittest.TestCase):
    def setUp(self):
        self.camera = CameraUser(None)

    def tearDown(self):
        self.camera = None

    def test_camera_interface_(self):
        expected = Schema()
        (
            STATE_ELEMENT(expected).key("state")
            .displayedName("State")
            .initialValue(State.UNKNOWN)
            .commit(),
        )
        self.camera.expectedParameters(expected)

        self.assertTrue(expected.has('state'))
        states = expected.getOptions('state')
        self.assertTrue('INIT' in states)
        self.assertTrue('UNKNOWN' in states)
        self.assertTrue('ERROR' in states)
        self.assertTrue('ACQUIRING' in states)
        self.assertTrue('ON' in states)
        self.assertTrue(expected.has('connectCamera'))
        self.assertTrue(expected.has('acquire'))
        self.assertTrue(expected.has('trigger'))
        self.assertTrue(expected.has('stop'))
        self.assertTrue(expected.has('resetHardware'))
        self.assertTrue(expected.has('output.schema.data.image'))
        self.assertTrue(expected.has('exposureTime'))
        self.assertTrue(expected.has('imageStorage'))

if __name__ == '__main__':
    unittest.main()
