"""
These tests are part of the karabo.usermacros API.
To run all tests, execute:
  nosetests -v
"""

from contextlib import contextmanager
import unittest
import numpy as np

from karabo.usermacro_api.middlelayer import (
    Device, DeviceTest, Slot, sync_tst)
from karabo.usermacros import (AMesh, APathScan, AScan, DScan, GenericProxy,
                               meshTrajectory, splitTrajectory)


class Kwargator(object):
    def __init__(self, **kwargs):
        self.__dict__.update(kwargs)


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

    def test_len_2_1D_trajectory_split_2(self):
        """Test the split in 2 of a 2 points 1D trajectory"""
        result = [point for point in splitTrajectory([0, 20], 2)]
        self.assertEqual(result, [(0, True), (10, True), (20, True)])

    def test_len_2_3D_rectilinear_trajectory_split_4(self):
        """Test the split in 4 of a 2 points 3D trajectory"""
        result = [(tuple(point), pause)
                  for point, pause in
                  splitTrajectory([(0, 0, 0), (1, 1, 1),
                                   (20, 20, 20)], 4)]

        oracle = [((0, 0, 0), True), ((1, 1, 1), False),
                  ((5, 5, 5), True), ((10, 10, 10), True),
                  ((15, 15, 15), True), ((20, 20, 20), True)]

        self.assertEqual(result, oracle)

    def test_len_4_3D_rectilinear_trajectory_split_2(self):
        """Test the split in 2 of a 4 points 3D trajectory"""
        result = [(tuple(point), pause)
                  for point, pause in
                  splitTrajectory([(0, -5, 1), (0, 0, 1),
                                   (10, 0, 1), (10, -5, 1)], 2)]
        oracle = [((0, -5, 1), True), ((0, 0, 1), False), ((5, 0, 1), True),
                  ((10, 0, 1), False), ((10, -5, 1), True)]

        self.assertEqual(result, oracle)

    def test_len_3_1D_trajectory_split_2(self):
        """Test the split in 2 of a 3 points 1D trajectory"""
        result = [point for point in splitTrajectory([1, 10, 20], 2)]
        oracle = [(1, True), (10, False), (10.5, True), (20, True)]

        self.assertEqual(result, oracle)

    def test_len_3_1D_trajectory_split_177(self):
        """Test against linspace the split in 177 of a 1D trajectory"""
        traj = [1, 10, 20]
        num = 177
        result = []
        for point, pause in splitTrajectory(traj, num):
            if pause:
                result.append(point)

        oracle = list(np.linspace(traj[0], traj[-1], num=num+1))
        self.assertEqual(len(result), len(oracle))

        for res, ora in zip(result, oracle):
            self.assertAlmostEqual(res, ora)

    def test_mesh_trajectory(self):
        """Test the generation of a mesh trajectory"""
        result = [point for point in meshTrajectory(range(3), range(3))]
        oracle = [(0, 0), (0, 1), (0, 2),
                  (1, 2), (1, 1), (1, 0),
                  (2, 0), (2, 1), (2, 2)]

        self.assertEqual(result, oracle)

    @sync_tst
    def test_scans_initializations(self):
        """Test the initialization of all different scan objects"""
        expected_rep = ("AScan('tm1', [(0, 0), (10, 10), (15, 15)], "
                        "'lsim', 5.0, steps=True, number_of_steps=0)")

        ascaney = AScan(self.m1, self.pos1, self.sens, self.expo)
        self.assertEqual(type(ascaney), AScan)
        self.assertEqual(ascaney.__repr__(), expected_rep)

        expected_rep = ("AMesh('tm1', [(0, 0), (10, 10), (15, 15)], "
                        "[(15, 15), (10, 10), (0, 0)], 'lsim', 5.0, steps=True, "
                        "number_of_steps=0)")
        ameshy = AMesh(self.m1, self.pos1, self.pos2, self.sens, self.expo)
        self.assertEqual(type(ameshy), AMesh)
        self.assertEqual(ameshy.__repr__(), expected_rep)

        expected_rep = ("APathScan('tm1', [(0, 0), (10, 10), (15, 15)], "
                        "'lsim', 5.0, steps=True, number_of_steps=0)")
        # Pun intented
        apathy = APathScan(self.m1, self.pos1, self.sens, self.expo)
        self.assertEqual(type(apathy), APathScan)
        self.assertEqual(apathy.__repr__(), expected_rep)

        self.m1._proxy.encoderPosition = Kwargator(magnitude=10)

        expected_rep = ("DScan('tm1', [(0, 0), (10, 10), (15, 15)], "
                        "'lsim', 5.0, steps=True, number_of_steps=0)")
        dscaney = DScan(self.m1, self.pos1, self.sens, self.expo)
        self.assertEqual(type(dscaney), DScan)
        self.assertEqual(dscaney.__repr__(), expected_rep)


if __name__ == "__main__":
    unittest.main()
