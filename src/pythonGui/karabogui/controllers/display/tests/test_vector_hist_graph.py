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

from karabo.native import Configurable, Double, Hash, NDArray, VectorFloat
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..vector_hist_graph import VectorHistogramGraph


class VectorObject(Configurable):
    prop = VectorFloat(defaultValue=[2.3, 4.5, 7.9])


class NDArrayObject(Configurable):
    prop = NDArray(
        defaultValue=[2.3, 4.5, 7.9, 1.0, 2.0, 3.0, 4.0, 3.0, 9.0, 10.1],
        shape=(10,),
        dtype=Double)


@pytest.fixture
def vector_hist_setup(gui_app):
    schema = VectorObject.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    controller = VectorHistogramGraph(proxy=proxy)
    controller.create(None)
    assert controller.widget is not None
    yield controller, proxy
    controller.destroy()
    assert controller.widget is None


def test_vector_hist_basics(vector_hist_setup):
    controller, proxy = vector_hist_setup
    controller.model.auto = False
    controller.model.start = 0
    controller.model.stop = 10
    set_proxy_value(proxy, "prop", [1, 2, 2, 3, 3, 3, 3, 8])
    curve = controller._plot
    x, y = curve.getData()
    np.testing.assert_array_almost_equal(
        x, [-0.5, 0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5])
    np.testing.assert_array_almost_equal(y, [0, 1, 2, 4, 0, 0, 0, 0, 1, 0])


def test_vector_hist_empty(vector_hist_setup):
    controller, proxy = vector_hist_setup
    controller.model.auto = False
    controller.model.start = 0
    controller.model.stop = 10
    set_proxy_value(proxy, "prop", [])
    curve = controller._plot
    x, y = curve.getData()
    assert x is None
    assert y is None


def test_array_hist_basics(gui_app):
    # setup
    schema = NDArrayObject.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    controller = VectorHistogramGraph(proxy=proxy)
    controller.create(None)
    assert controller.widget is not None

    # test body
    controller.model.auto = False
    controller.model.start = 0
    controller.model.stop = 10
    value = np.array([1.0, 2.0, 2.0, 3.0, 3.0, 3.0, 3.0, 8.1, 9.2, 10.1],
                     dtype=np.float64)
    array_hash = Hash("type", 20,
                      "data", value.tobytes())
    set_proxy_value(proxy, "prop", array_hash)

    curve = controller._plot
    x, y = curve.getData()
    np.testing.assert_array_almost_equal(
        x, [-0.5, 0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5])
    np.testing.assert_array_almost_equal(y, [9, 1, 9, 0, 0, 0, 0, 0, 0, 0])

    # teardown
    controller.destroy()
    assert controller.widget is None
