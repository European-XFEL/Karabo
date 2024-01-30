# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
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
        assert threshold == TYPICAL
        threshold = _get_sample_threshold(10000)
        assert threshold == TYPICAL
        threshold = _get_sample_threshold(200000)
        assert threshold == TYPICAL
        threshold = _get_sample_threshold(200001)
        assert threshold == 30000
        threshold = _get_sample_threshold(300000)
        assert threshold == 30000
        threshold = _get_sample_threshold(600000)
        assert threshold == 60000
