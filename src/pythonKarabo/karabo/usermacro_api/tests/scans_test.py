"""
These tests are part of the karabo.usermacros API.
To run all tests, excecute:
   python -m unittest discover -p *_test.py
"""
from contextlib import contextmanager
import unittest

from karabo.middlelayer import Device, Slot
from karabo.middlelayer_api.tests.eventloop import DeviceTest, sync_tst

from karabo.usermacros import AMesh, APathScan, AScan, DScan, GenericProxy

class Kwargator(object):
    def __init__(self, **kwargs):
        self.__dict__.update(kwargs)

class TestDev(Device):

    def __init__(self, configuration):
        super().__init__(configuration)
        self.lockedBy = ""
        self._lock_count = 0
        self.encoderPosition = Kwargator(magnitude = 10)

    @Slot()
    def ping(self):
        print("Ping!")


def getMockDevice(name, **kwargs):
    cls = type(name, (TestDev,), {})
    return cls(kwargs)


class Tests(DeviceTest):
    """The tests in this class run on behalf of the device "local".

    As this class inherits from :class:`DeviceTest`, it is possible to provide
    a :func:`lifetimeManager`, such that the test can be run within an
    eventloop
    """

    @classmethod
    @contextmanager
    def lifetimeManager(cls):

        cls.local = TestDev({"_deviceId_": "Scans_UnitTests"})

        cls.tm1 = getMockDevice("BeckhoffSimpleMotor",
                                _deviceId_="tm1",
                                stepLength=0)
        cls.tm2 = getMockDevice("BeckhoffSimpleMotor",
                                _deviceId_="tm2",
                                stepLength=0)
        cls.tm3 = getMockDevice("BeckhoffSimpleMotor",
                                _deviceId_="tm3",
                                stepLength=0)
        cls.lsim = getMockDevice("LimaSimulatedCamera",
                                 _deviceId_="lsim",
                                 cameraType="Simulator")
        with cls.deviceManager(cls.lsim, cls.tm2, cls.tm3,
                               cls.tm1, lead=cls.local):
            yield

    @sync_tst
    def setUp(self):
        self.m1 = GenericProxy('tm1')
        self.pos1 = [(0, 0), (10, 10), (15, 15)]
        self.pos2 = [(15, 15), (10, 10), (0, 0)]
        self.sens = GenericProxy('lsim')
        self.expo = 5

    @sync_tst
    def test_scans_initializations(self):
        expected_rep = ("AScan('tm1', [(0, 0), (10, 10), (15, 15)], "
                        "'lsim', 5, steps=True, number_of_steps=0)")

        ascaney = AScan(self.m1, self.pos1, self.sens, self.expo)
        self.assertEqual(type(ascaney), AScan)
        self.assertEqual(ascaney.__repr__(), expected_rep)

        expected_rep = ("AMesh('tm1', [(0, 0), (10, 10), (15, 15)], "
                        "[(15, 15), (10, 10), (0, 0)], 'lsim', 5, steps=True, "
                        "number_of_steps=0)")
        ameshy = AMesh(self.m1, self.pos1, self.pos2, self.sens, self.expo)
        self.assertEqual(type(ameshy), AMesh)
        self.assertEqual(ameshy.__repr__(), expected_rep)

        expected_rep = ("APathScan('tm1', [(0, 0), (10, 10), (15, 15)], "
                        "'lsim', 5, steps=True, number_of_steps=0)")
        apathy = APathScan(self.m1, self.pos1, self.sens, self.expo) # Pun int.
        self.assertEqual(type(apathy), APathScan)
        self.assertEqual(apathy.__repr__(), expected_rep)

        expected_rep = ("DScan('tm1', [(0, 0), (10, 10), (15, 15)], "
                        "'lsim', 5, steps=True, number_of_steps=0)")
        dscaney = DScan(self.m1, self.pos1, self.sens, self.expo)
        self.assertEqual(type(dscaney), DScan)
        self.assertEqual(dscaney.__repr__(), expected_rep)


if __name__ == "__main__":
    unittest.main()
