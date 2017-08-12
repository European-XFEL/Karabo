"""
These tests are part of the karabo.usermacros API.
To run all tests, execute:
  nosetests -v
"""

import unittest
import numpy as np

from karabo.middlelayer_api.tests.eventloop import DeviceTest, sync_tst
from karabo.usermacros import meshTrajectory, splitTrajectory


class Tests(DeviceTest):
    """Test the Scan classes"""

    @sync_tst
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
        print(result)
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

if __name__ == "__main__":
    unittest.main()
