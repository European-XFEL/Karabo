"""
These tests are part of the karabo.usermacros API.
To run all tests, execute:
   python -m unittest discover -p *_test.py
"""

from contextlib import contextmanager
import unittest

from karabo.usermacro_api.middlelayer import (
    Device, KaraboError, Slot, DeviceTest, sync_tst)
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


class TestXDetectorAsSensible(Sensible):
    """Test generic proxy for X-ray detectors"""


class TestDiodeAsSensible(TestXDetectorAsSensible):
    """Test generic proxy for diodes"""
    generalizes = ("Diode",)


class TestXGMAsSensible(TestXDetectorAsSensible):
    """Test generic proxy for XGM"""
    generalizes = ("Xgm",)


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
        cls.diode1 = getMockDevice("Diode",
                                   _deviceId_="diode1",
                                   cameraTye="TestDiode")
        cls.xgm1 = getMockDevice("Xgm",
                                 _deviceId_="xgm1",
                                 cameraType="TestXgm")
        cls.unknown = getMockDevice("UnknownClassId",
                                    _deviceId_="unknown")
        with cls.deviceManager(cls.diode1, cls.lsim, cls.tm2, cls.tm3,
                               cls.tm1, cls.unknown, cls.xgm1, lead=cls.local):
            yield

    @sync_tst
    def test_single_device_instantiation(self):
        output = GenericProxy('tm1')

        self.assertIsInstance(output, BeckhoffMotorAsMovable)
        self.assertEqual(repr(output), "BeckhoffMotorAsMovable('tm1')")
        self.assertEqual(output.getBoundDevices(), 'tm1')

        output = GenericProxy('lsim')
        self.assertIsInstance(output, CamAsSensible)
        self.assertEqual(repr(output), "CamAsSensible('lsim')")
        self.assertEqual(output.getBoundDevices(), 'lsim')

    @sync_tst
    def test_invalid_input_instantiation(self):
        with self.assertRaises(KaraboError):
            GenericProxy(42)

    @sync_tst
    def test_no_input_instantiation(self):
        output = GenericProxy()
        self.assertEqual(output, None)

    @sync_tst
    def test_single_container_instantiation(self):
        containerRep = "Movable(BeckhoffMotorAsMovable('tm1'))"

        output = Movable(Movable('tm1'))
        self.assertIsInstance(output, Movable)
        self.assertEqual(repr(output), containerRep)
        self.assertEqual(output.getBoundDevices(), ['tm1'])

        output = GenericProxy(Movable('tm1'))
        self.assertIsInstance(output, Movable)
        self.assertEqual(repr(output), containerRep)
        self.assertEqual(output.getBoundDevices(), ['tm1'])

        output = Movable(GenericProxy('tm1'))
        self.assertIsInstance(output, Movable)
        self.assertEqual(repr(output), containerRep)
        self.assertEqual(output.getBoundDevices(), ['tm1'])

        output = GenericProxy(GenericProxy('tm1'))
        self.assertIsInstance(output, Movable)
        self.assertEqual(repr(output), containerRep)
        self.assertEqual(output.getBoundDevices(), ['tm1'])

    @sync_tst
    def test_triplet_instantiation(self):
        tripletRep = ("Movable(BeckhoffMotorAsMovable('tm1'), "
                      "BeckhoffMotorAsMovable('tm2'), "
                      "BeckhoffMotorAsMovable('tm3'))")

        output = GenericProxy('tm1', 'tm2', 'tm3')
        self.assertIsInstance(output, Movable)
        self.assertEqual(repr(output), tripletRep)
        self.assertEqual(output.getBoundDevices(), ['tm1', 'tm2', 'tm3'])

        output = Movable('tm1', 'tm2', 'tm3')
        self.assertIsInstance(output, Movable)
        self.assertEqual(repr(output), tripletRep)
        self.assertEqual(output.getBoundDevices(), ['tm1', 'tm2', 'tm3'])

    @sync_tst
    def test_triplet_container_instantiation(self):
        containerRep = ("Movable(BeckhoffMotorAsMovable('tm1'), "
                        "Movable(BeckhoffMotorAsMovable('tm2'), "
                        "BeckhoffMotorAsMovable('tm3')))")

        output = GenericProxy('tm1', Movable('tm2', 'tm3'))
        self.assertIsInstance(output, Movable)
        self.assertEqual(repr(output), containerRep)
        self.assertEqual(output.getBoundDevices(), ['tm1', ['tm2', 'tm3']])

        output = GenericProxy('tm1', GenericProxy('tm2', 'tm3'))
        self.assertIsInstance(output, Movable)
        self.assertEqual(repr(output), containerRep)
        self.assertEqual(output.getBoundDevices(), ['tm1', ['tm2', 'tm3']])

    @sync_tst
    def test_triplet_container_hierarchy_instantiation(self):
        """Test the instantiation of a container of a hierarchy of sensibles"""
        containerRep = ("Sensible(CamAsSensible('lsim'), "
                        "TestXDetectorAsSensible("
                        "TestDiodeAsSensible('diode1'), "
                        "TestXGMAsSensible('xgm1')))")

        output = GenericProxy('lsim', Sensible('diode1', 'xgm1'))
        self.assertIsInstance(output, Sensible)
        self.assertEqual(repr(output), containerRep)
        self.assertEqual(output.getBoundDevices(),
                         ['lsim', ['diode1', 'xgm1']])

        output = GenericProxy('lsim', GenericProxy('diode1', 'xgm1'))
        self.assertIsInstance(output, Sensible)
        self.assertEqual(repr(output), containerRep)
        self.assertEqual(output.getBoundDevices(),
                         ['lsim', ['diode1', 'xgm1']])

        output = Sensible('lsim', TestXDetectorAsSensible('diode1', 'xgm1'))
        self.assertIsInstance(output, Sensible)
        self.assertEqual(repr(output), containerRep)
        self.assertEqual(output.getBoundDevices(),
                         ['lsim', ['diode1', 'xgm1']])

    @sync_tst
    def test_paramregex_instantiation(self):
        """Test the support of regex parameter"""
        output = GenericProxy('tm1@.?tate$')

        self.assertIsInstance(output, BeckhoffMotorAsMovable)
        self.assertEqual(output.value, {'state': output._proxy.state})

        output = Sensible('lsim@(^(?i)deviceid$|^(?i)classid$)')
        self.assertIsInstance(output, CamAsSensible)
        self.assertEqual(
            output.value,
            {'deviceId': 'lsim', 'classId': 'LimaSimulatedCamera'})

        output = Sensible('lsim:classid')
        self.assertIsInstance(output, CamAsSensible)
        self.assertEqual(
            output.value,
            {'classId': 'LimaSimulatedCamera'})

    @sync_tst
    def test_wrong_type_instantiation(self):
        output = Movable('lsim')
        self.assertIsInstance(output, Movable)

        output = Sensible('unknown')
        self.assertIsInstance(output, Sensible)

        output = Sensible('lsim', 'unknown')
        self.assertIsInstance(output, Sensible)

        containerRep = ("Sensible(CamAsSensible('lsim'), "
                        "Sensible('unknown'))")
        self.assertEqual(repr(output), containerRep)
        self.assertEqual(output.getBoundDevices(), ['lsim', 'unknown'])

        output = Movable('lsim', 'unknown')
        self.assertIsInstance(output, Movable)

        containerRep = ("Movable(Movable('lsim'), "
                        "Movable('unknown'))")
        self.assertEqual(repr(output), containerRep)
        self.assertEqual(output.getBoundDevices(), ['lsim', 'unknown'])

        with self.assertRaises(KaraboError):
            GenericProxy('unknown')

        with self.assertRaises(KaraboError):
            Sensible('lsim', GenericProxy('unknown'))

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
