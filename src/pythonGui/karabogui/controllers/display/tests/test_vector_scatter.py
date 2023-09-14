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

from karabo.common.scenemodel.api import VectorScatterGraphModel
from karabo.native import Configurable, VectorFloat, VectorInt32
from karabogui.binding.proxy import PropertyProxy
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..vector_scatter_graph import DisplayVectorScatterGraph


class Object(Configurable):
    x = VectorFloat(defaultValue=[2.3, 4.5, 7.9])
    y = VectorInt32(defaultValue=[1, 2, 3])


@pytest.fixture
def vector_scatter_setup(gui_app):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "x")
    device = proxy.root_proxy
    prop_proxy = PropertyProxy(root_proxy=device, path="y")
    controller = DisplayVectorScatterGraph(proxy=proxy)
    controller.create(None)
    controller.visualize_additional_property(prop_proxy)
    assert controller.widget is not None

    yield controller, proxy, prop_proxy

    controller.destroy()
    assert controller.widget is None


def test_vector_scatter_graph_basics(vector_scatter_setup):
    controller, proxy, prop_proxy = vector_scatter_setup
    set_proxy_value(proxy, "x", [2.3, 4.5, 7.9])
    set_proxy_value(prop_proxy, "y", [1, 2, 3])
    curve = controller._plot
    x, y = curve.getData()
    np.testing.assert_array_almost_equal(x, [2.3, 4.5, 7.9])
    np.testing.assert_array_almost_equal(y, [1, 2, 3])

    # y is different
    set_proxy_value(proxy, "x", [2.3, 4.5, 7.9])
    set_proxy_value(prop_proxy, "y", [1, 2, 3, 4])
    x, y = curve.getData()
    np.testing.assert_array_almost_equal(x, [2.3, 4.5, 7.9])
    np.testing.assert_array_almost_equal(y, [1, 2, 3])

    # x is different
    set_proxy_value(proxy, "x", [2.3, 4.5])
    x, y = curve.getData()
    np.testing.assert_array_almost_equal(x, [2.3, 4.5, 7.9])
    np.testing.assert_array_almost_equal(y, [1, 2, 3])
    # No replot happened, y provides the update, we have size 2 on x
    set_proxy_value(prop_proxy, "y", [1, 2, 3, 4])
    x, y = curve.getData()
    np.testing.assert_array_almost_equal(x, [2.3, 4.5])
    np.testing.assert_array_almost_equal(y, [1, 2])


def test_vector_scatter_graph_pointsize(vector_scatter_setup, mocker):
    _, proxy, prop_proxy = vector_scatter_setup
    controller = DisplayVectorScatterGraph(proxy=proxy,
                                           model=VectorScatterGraphModel())
    controller.create(None)
    action = controller.widget.actions()[10]
    assert action.text() == "Point Size"

    dsym = ("karabogui.controllers.display."
            "vector_scatter_graph.QInputDialog")
    QInputDialog = mocker.patch(dsym)
    QInputDialog.getDouble.return_value = 2.7, True
    action.trigger()
    assert controller.model.psize == 2.7
    assert controller._plot.opts["size"] == 2.7

    controller.destroy()


def test_removal_property(vector_scatter_setup):
    _, proxy, prop_proxy = vector_scatter_setup
    controller = DisplayVectorScatterGraph(proxy=proxy,
                                           model=VectorScatterGraphModel())
    controller.create(None)
    controller.visualize_additional_property(prop_proxy)
    set_proxy_value(proxy, "x", [2.3, 4.5, 7.9])
    set_proxy_value(prop_proxy, "y", [1, 2, 3])

    curve = controller._plot
    x, y = curve.getData()
    np.testing.assert_array_almost_equal(x, [2.3, 4.5, 7.9])
    np.testing.assert_array_almost_equal(y, [1, 2, 3])

    assert controller.remove_additional_property(prop_proxy)
    assert controller._y_proxy is None
    # Removing more does not hurt
    assert not controller.remove_additional_property(prop_proxy)
