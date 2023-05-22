# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import unittest

from karabogui.graph.plots.utils import _get_sample_threshold

TYPICAL = 20000


class TestPlotUtils(unittest.TestCase):

    def setUp(self):
        super().setUp()

    def tearDown(self):
        super().tearDown()

    def test_sample_threshold(self):
        """Test the sample threshold finding"""
        threshold = _get_sample_threshold(1000)
        self.assertEqual(threshold, TYPICAL)
        threshold = _get_sample_threshold(10000)
        self.assertEqual(threshold, TYPICAL)
        threshold = _get_sample_threshold(200000)
        self.assertEqual(threshold, TYPICAL)
        threshold = _get_sample_threshold(200001)
        self.assertEqual(threshold, 30000)
        threshold = _get_sample_threshold(300000)
        self.assertEqual(threshold, 30000)
        threshold = _get_sample_threshold(600000)
        self.assertEqual(threshold, 60000)
