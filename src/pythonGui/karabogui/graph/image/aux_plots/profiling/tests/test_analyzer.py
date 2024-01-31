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

import numpy as np
import pytest

from karabogui.graph.common.api import ImageRegion

from ..analyzer import ProfileAnalyzer

X_LENGTH, Y_LENGTH = (5, 4)


@pytest.fixture()
def analyzer(gui_app):
    profiler = ProfileAnalyzer()
    yield profiler


def test_basics(analyzer):
    assert not analyzer.smooth
    assert analyzer.axis_data is None
    assertArrayIsEmpty(analyzer._x_profile)
    assertArrayIsEmpty(analyzer._y_profile)
    assertArrayIsEmpty(analyzer._x_fit)
    assertArrayIsEmpty(analyzer._y_fit)
    assertArrayIsEmpty(analyzer._fit_params)


def test_analyze(analyzer):
    # Check default, fails without axis data
    analyzer.analyze(_get_region(valid=True))
    assertArrayIsEmpty(analyzer._x_profile)
    assertArrayIsEmpty(analyzer._y_profile)
    analyzer.axis_data = np.arange(X_LENGTH)
    analyzer.analyze(_get_region(valid=True), axis=0)
    assertArrayIsNotEmpty(analyzer._x_profile)
    assertArrayIsNotEmpty(analyzer._y_profile)
    analyzer.analyze(_get_region(valid=False))
    assertArrayIsEmpty(analyzer._x_profile)
    assertArrayIsEmpty(analyzer._y_profile)


def test_fit(analyzer):
    # Check default, fails without axis data and profile data
    analyzer.fit()
    assertArrayIsEmpty(analyzer._x_fit)
    assertArrayIsEmpty(analyzer._y_fit)
    assert analyzer._fit_params is None
    analyzer.axis_data = np.arange(X_LENGTH)
    analyzer.fit()
    assertArrayIsEmpty(analyzer._x_fit)
    assertArrayIsEmpty(analyzer._y_fit)
    assert analyzer._fit_params is None
    analyzer.analyze(_get_region(valid=True), axis=0)
    analyzer.fit()
    assertArrayIsNotEmpty(analyzer._x_fit)
    assertArrayIsNotEmpty(analyzer._y_fit)
    assert analyzer._fit_params is not None
    analyzer.analyze(_get_region(valid=False), axis=0)
    analyzer.fit()
    assertArrayIsEmpty(analyzer._x_fit)
    assertArrayIsEmpty(analyzer._y_fit)
    assert analyzer._fit_params is None


def test_get_stats(analyzer):
    # Check default, fails without axis data and profile data
    assert analyzer.stats == {}
    analyzer.axis_data = np.arange(X_LENGTH)
    assert analyzer.stats == {}
    analyzer.analyze(_get_region(valid=True), axis=0)
    analyzer.fit()
    stats = analyzer.stats
    assert stats != {}
    assert stats.get("fwhm") is not None
    analyzer.analyze(_get_region(valid=False), axis=0)
    analyzer.fit()
    stats = analyzer.stats
    assert stats == {}
    assert stats.get("fwhm") is None


def _get_region(valid=True):
    region = ImageRegion()
    if valid:
        image = np.arange(X_LENGTH * Y_LENGTH).reshape(Y_LENGTH, X_LENGTH)
        region = ImageRegion(region=image, region_type=ImageRegion.Area,
                             x_slice=slice(X_LENGTH), y_slice=slice(Y_LENGTH))
        assert region.valid() is valid

    return region


def assertArrayIsEmpty(array):
    np.testing.assert_array_equal(array, [])


def assertArrayIsNotEmpty(array):
    np.testing.assert_raises(AssertionError, np.testing.assert_array_equal,
                             array, [])
