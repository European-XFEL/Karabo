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
import pytest

from karabo.common.scenemodel.api import ScatterGraphModel
from karabo.native import Configurable, Double, Hash, Timestamp
from karabogui.binding.proxy import PropertyProxy
from karabogui.const import IS_WINDOWS_SYSTEM
from karabogui.testing import (
    get_class_property_proxy, set_proxy_hash, set_proxy_value)

from ..scatter_graph import DisplayScatterGraph


class Object(Configurable):
    x = Double()
    y = Double()


@pytest.fixture
def scatter_graph_setup(gui_app):
    schema = Object.getClassSchema()
    x = get_class_property_proxy(schema, "x")
    device = x.root_proxy
    y = PropertyProxy(root_proxy=device, path="y")
    controller = DisplayScatterGraph(proxy=x)
    controller.create(None)
    controller.visualize_additional_property(y)
    assert controller.widget is not None
    yield controller, x, y

    controller.destroy()
    assert controller.widget is None


@pytest.mark.skipif(IS_WINDOWS_SYSTEM,
                    reason="curve.getData returns empty arrays in Windows")
def test_scatter_graph_basics(scatter_graph_setup):
    controller, x, y = scatter_graph_setup
    set_proxy_value(x, "x", 2.1)
    set_proxy_value(y, "y", 3.2)
    curve = controller._plot
    assert list(curve.getData()) == [2.1, 3.2]


@pytest.mark.skipif(IS_WINDOWS_SYSTEM,
                    reason="curve.getData returns empty arrays in Windows")
def test_scatter_timestamp(scatter_graph_setup):
    controller, x, y = scatter_graph_setup
    timestamp = Timestamp()
    set_proxy_value(x, "x", 2.1)
    h = Hash("y", 3.2)
    set_proxy_hash(y, h, timestamp)
    curve = controller._plot
    assert list(curve.getData()) == [2.1, 3.2]
    h = Hash("y", 13.2)
    set_proxy_hash(y, h, timestamp)
    assert list(curve.getData()) == [2.1, 3.2]


def test_deque(mocker, scatter_graph_setup):
    controller, x, _ = scatter_graph_setup
    controller = DisplayScatterGraph(proxy=x, model=ScatterGraphModel())
    controller.create(None)
    action = controller.widget.actions()[10]
    assert action.text() == "Queue Size"

    path = "karabogui.controllers.display.scatter_graph.QInputDialog.getInt"
    mocker.patch(path, return_value=(20, True))
    action.trigger()
    assert controller.model.maxlen == 20
    assert controller._x_values.maxlen == 20
    assert controller._y_values.maxlen == 20


def test_pointsize(mocker, scatter_graph_setup):
    controller, x, _ = scatter_graph_setup
    controller = DisplayScatterGraph(proxy=x, model=ScatterGraphModel())
    controller.create(None)
    action = controller.widget.actions()[11]
    assert action.text() == "Point Size"

    path = "karabogui.controllers.display.scatter_graph.QInputDialog.getDouble"
    mocker.patch(path, return_value=(2.7, True))
    action.trigger()
    assert controller.model.psize == 2.7
    assert controller._plot.opts["size"] == 2.7


def test_removal_property(scatter_graph_setup):
    controller, x, y = scatter_graph_setup
    controller = DisplayScatterGraph(proxy=x, model=ScatterGraphModel())
    controller.create(None)
    controller.visualize_additional_property(y)
    # Put a few values
    set_proxy_value(x, "x", 2.1)
    set_proxy_value(y, "y", 3.2)
    set_proxy_value(x, "x", 5.3)
    set_proxy_value(y, "y", 4.6)

    if not IS_WINDOWS_SYSTEM:
        assert controller._last_x_value is not None
        assert len(controller._x_values) > 0
        assert len(controller._y_values) > 0

    assert controller.remove_additional_property(y)
    assert controller._y_proxy is None
    assert controller._last_x_value is None
    assert len(controller._x_values) == 0
    assert len(controller._y_values) == 0

    assert not controller.remove_additional_property(y)
