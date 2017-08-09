"""
These tests are part of the karabo.usermacros API.
To run all tests, excecute:
   python -m unittest discover *_test.py
"""

import unittest
from contextlib import contextmanager

from karabo.middlelayer import Device, KaraboError, Slot
from karabo.middlelayer_api.tests.eventloop import DeviceTest, sync_tst

from karabo.usermacros import (BeckhoffMotorAsMovable, CamAsSensible,
                               GenericProxy, Movable, Sensible)


class TestDev(Device):

    def __init__(self, configuration):
        super().__init__(configuration)
        self.lockedBy = ""
        self._lock_count = 0

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

        cls.local = TestDev({"_deviceId_": "GenericProxy_UnitTests"})

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
    def test_single_device_instantiation(self):
        output = GenericProxy('tm1')

        self.assertIsInstance(output, BeckhoffMotorAsMovable)
        self.assertEqual(output.__repr__(), "BeckhoffMotorAsMovable('tm1')")

        output = GenericProxy('lsim')
        self.assertIsInstance(output, CamAsSensible)
        self.assertEqual(output.__repr__(), "CamAsSensible('lsim')")

    @sync_tst
    def test_invalid_input_instantiation(self):
        with self.assertRaises(KaraboError):
            GenericProxy(42)

    @sync_tst
    def test_single_container_instantiation(self):
        containerRep = "Movable(BeckhoffMotorAsMovable('tm1'))"

        output = Movable(Movable('tm1'))
        self.assertIsInstance(output, Movable)
        self.assertEqual(output.__repr__(), containerRep)

        output = GenericProxy(Movable('tm1'))
        self.assertIsInstance(output, Movable)
        self.assertEqual(output.__repr__(), containerRep)

        output = Movable(GenericProxy('tm1'))
        self.assertIsInstance(output, Movable)
        self.assertEqual(output.__repr__(), containerRep)

        output = GenericProxy(GenericProxy('tm1'))
        self.assertIsInstance(output, Movable)
        self.assertEqual(output.__repr__(), containerRep)

    @sync_tst
    def test_triplet_instantiation(self):
        tripletRep = ("Movable(BeckhoffMotorAsMovable('tm1'), "
                      "BeckhoffMotorAsMovable('tm2'), "
                      "BeckhoffMotorAsMovable('tm3'))")

        output = GenericProxy('tm1', 'tm2', 'tm3')
        self.assertIsInstance(output, Movable)
        self.assertEqual(output.__repr__(), tripletRep)

        output = Movable('tm1', 'tm2', 'tm3')
        self.assertIsInstance(output, Movable)
        self.assertEqual(output.__repr__(), tripletRep)

    @sync_tst
    def test_triplet_container_instantiation(self):
        containerRep = ("Movable(BeckhoffMotorAsMovable('tm1'), "
                        "Movable(BeckhoffMotorAsMovable('tm2'), "
                        "BeckhoffMotorAsMovable('tm3')))")

        output = GenericProxy('tm1', Movable('tm2', 'tm3'))
        self.assertIsInstance(output, Movable)
        self.assertEqual(output.__repr__(), containerRep)

        output = GenericProxy('tm1', GenericProxy('tm2', 'tm3'))
        self.assertIsInstance(output, Movable)
        self.assertEqual(output.__repr__(), containerRep)

    @sync_tst
    def test_wrong_type_instantiation(self):
        with self.assertRaises(KaraboError):
            Movable('lsim')

    @sync_tst
    def test_wrong_type_mixes(self):
        with self.assertRaises(KaraboError):
            GenericProxy('tm1', 'lsim')

        with self.assertRaises(KaraboError):
            Movable('tm1', 'tm2', Sensible('lsim'))

    @sync_tst
    def test_timeout_error(self):
        with self.assertRaises(KaraboError):
            GenericProxy('some-non-existing-device')

    @sync_tst
    def test_lock(self):
        output = Sensible('lsim')
        output.lockon(self.local.deviceId)
        self.assertEqual(output._proxy.lockedBy, self.local.deviceId)
        output.lockoff()
        self.assertEqual(output._proxy.lockedBy, "")

    @sync_tst
    def test_multiple_locks(self):
        output = GenericProxy('tm1')

        output.lockon(self.local.deviceId)
        self.assertEqual(output._proxy.lockedBy, self.local.deviceId)
        output.lockon(self.local.deviceId)
        self.assertEqual(output._proxy.lockedBy, self.local.deviceId)
        output.lockon(self.local.deviceId)
        self.assertEqual(output._proxy.lockedBy, self.local.deviceId)

        output.lockoff()
        self.assertEqual(output._proxy.lockedBy, "")

    @sync_tst
    def test_lock_container(self):
        output = GenericProxy('tm1', Movable('tm2'), Movable('tm3'))
        output.lockon(self.local.deviceId)

        for gpxy in output._generic_proxies:
            self.assertEqual(gpxy._proxy.lockedBy, self.local.deviceId)

    @sync_tst
    def test_recursive_lock(self):
        output = GenericProxy('tm1', Movable('tm2', 'tm3'))
        output.lockon(self.local.deviceId)

        for gpxy in output._generic_proxies:
            if gpxy._generic_proxies:
                for in_gpxy in gpxy._generic_proxies:
                    self.assertEqual(in_gpxy._proxy.lockedBy,
                                     self.local.deviceId)
            else:
                self.assertEqual(gpxy._proxy.lockedBy, self.local.deviceId)


if __name__ == "__main__":
    unittest.main()
