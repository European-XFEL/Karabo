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
import warnings
from platform import system

import numpy as np
import pytest

from karabo.common.scenemodel.api import VectorGraphModel
from karabo.native import (
    Configurable, Hash, NDArray, UInt32, VectorBool, VectorFloat, VectorInt32)
from karabogui.binding.proxy import PropertyProxy
from karabogui.controllers.display.vector_graph import (
    DisplayNDArrayGraph, DisplayVectorGraph)
from karabogui.graph.plots.utils import generate_baseline
from karabogui.testing import (
    get_class_property_proxy, set_proxy_hash, set_proxy_value)


class ArrayObject(Configurable):
    prop = NDArray(
        displayedName="NDArray",
        dtype=UInt32,
        shape=(10,))


class VectorObject(Configurable):
    prop = VectorFloat()
    value = VectorInt32(defaultValue=[1, 2, 3])
    bool_value = VectorBool()


def _get_controller_action(controller, text):
    # Get the x-transformation action
    for action in controller.widget.actions():
        if action.text() == text:
            return action


@pytest.fixture
def vector_graph_setup(gui_app):
    # setup
    schema = VectorObject.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    device = proxy.root_proxy
    controller = DisplayVectorGraph(proxy=proxy)
    controller.create(None)
    assert controller.widget is not None
    # yield
    yield controller, proxy, device
    # teardown
    controller.destroy()
    assert controller.widget is None


@pytest.mark.skipif(system() == "Windows",
                    reason="Crosshair does not toggle off with emit==False")
def test_crosshair(vector_graph_setup):
    controller, _, _ = vector_graph_setup
    cross_target = controller.widget._cross_target

    def assert_crosshair_exists(horizontal, vertical):
        assert (cross_target.h_line is not None) is horizontal
        assert (cross_target.v_line is not None) is vertical

    assert_crosshair_exists(False, False)
    cross_target.action_button.clicked.emit(True)
    assert_crosshair_exists(True, True)
    cross_target.action_button.clicked.emit(False)
    assert_crosshair_exists(False, False)


def test_set_value(vector_graph_setup):
    controller, proxy, _ = vector_graph_setup
    assert len(controller._curves) == 1
    curve = controller._curves.get(proxy)
    assert curve is not None
    value = [2, 4, 6]
    set_proxy_value(proxy, "prop", value)
    assert list(curve.yData) == value


def test_set_value_inf(vector_graph_setup):
    controller, proxy, _ = vector_graph_setup
    assert len(controller._curves) == 1
    curve = controller._curves.get(proxy)
    assert curve is not None
    value = [2, 4, 6]
    set_proxy_value(proxy, "prop", value)
    assert list(curve.yData) == value

    value = [2, np.inf, 6]
    set_proxy_value(proxy, "prop", value)
    assert list(curve.yData) == value

    value = [np.inf, np.inf, np.inf]
    set_proxy_value(proxy, "prop", value)

    # None curve in PyQtGraph >= 0.11.1
    assert list(curve.yData) == value

    value = [np.nan, np.nan, np.nan]
    with warnings.catch_warnings(record=True):
        warnings.simplefilter("always")
        set_proxy_value(proxy, "prop", value)
        # Note: Older PyQtGraph version <= 0.11.0 are not catching all
        # NaN values.
        np.testing.assert_almost_equal(list(curve.yData), value)


def test_visualize_prop(vector_graph_setup):
    controller, proxy, device = vector_graph_setup
    value_proxy = PropertyProxy(root_proxy=device, path="value")

    controller.visualize_additional_property(value_proxy)
    assert len(controller._curves) == 2
    assert len(controller.proxies) == 2

    curve = controller._curves.get(value_proxy)
    assert curve is not None
    value = [6, 12, 6]
    set_proxy_value(value_proxy, "value", value)
    assert list(curve.yData) == value

    assert controller.remove_additional_property(value_proxy)
    assert len(controller._curves) == 1
    assert len(controller.proxies) == 1


def test_set_bool_value(vector_graph_setup):
    controller, _, device = vector_graph_setup
    bool_value = PropertyProxy(root_proxy=device, path="bool_value")

    controller.visualize_additional_property(bool_value)
    assert len(controller._curves) == 2
    curve = controller._curves.get(bool_value)
    assert curve is not None
    value = [False, False, True]
    set_proxy_value(bool_value, "bool_value", value)
    assert list(curve.yData) == [0, 0, 1]


def test_action_names(vector_graph_setup):
    _, proxy, _ = vector_graph_setup
    controller = DisplayVectorGraph(proxy=proxy, model=VectorGraphModel())
    controller.create(None)
    action = controller.widget.actions()[0]
    assert action.text() == "Grid X"
    action.trigger()
    assert controller.model.x_grid is False
    action.trigger()
    assert controller.model.x_grid is True

    action = controller.widget.actions()[1]
    assert action.text() == "Grid Y"
    action.trigger()
    assert controller.model.y_grid is False
    action.trigger()
    assert controller.model.y_grid is True

    action = controller.widget.actions()[2]
    assert action.text() == "Log X"
    action.trigger()
    assert controller.model.x_log is True
    action.trigger()
    assert controller.model.x_log is False

    action = controller.widget.actions()[3]
    assert action.text() == "Log Y"
    action.trigger()
    assert controller.model.y_log is True
    action.trigger()
    assert controller.model.y_log is False

    action = controller.widget.actions()[4]
    assert action.text() == "Invert X"
    action.trigger()
    assert controller.model.x_invert is True
    action.trigger()
    assert controller.model.x_invert is False

    action = controller.widget.actions()[5]
    assert action.text() == "Invert Y"
    action.trigger()
    assert controller.model.y_invert is True
    action.trigger()
    assert controller.model.y_invert is False

    controller.destroy()


def test_xtransform(vector_graph_setup, mocker):
    controller, proxy, _ = vector_graph_setup
    value = [1, 2, 3]
    set_proxy_value(proxy, "prop", value)

    # Get the action
    action = _get_controller_action(controller, "X-Transformation")
    assert action is not None

    offset = 20
    step = 10
    # Trigger the transformation configuration
    dialog = "karabogui.controllers.basearray.TransformDialog.get"
    content = {"offset": offset, "step": step}
    mocker.patch(dialog, return_value=(content, True))
    action.trigger()

    # Check if model is not
    for proxy, curve in controller._curves.items():
        x_expected = generate_baseline(proxy.value, offset, step)
        np.testing.assert_array_equal(curve.xData, x_expected)


def test_view_settings(vector_graph_setup, mocker):
    controller, _, _ = vector_graph_setup
    # Get the action
    action = _get_controller_action(controller, "View")
    assert action is not None

    # Trigger the transformation configuration
    dialog = "karabogui.graph.plots.dialogs.view.GraphViewDialog.get"
    content = {"title": "TestTitle", "background": "white"}
    mocker.patch(dialog, return_value=(content, True))
    action.trigger()

    config = controller.widget.configuration
    assert config["title"] == "TestTitle"
    assert config["background"] == "white"


def test_arrays_graph_set_value(gui_app):
    # set up
    schema = ArrayObject.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    controller = DisplayNDArrayGraph(proxy=proxy)
    controller.create(None)
    assert controller.widget is not None

    # test body
    curve = controller._curves.get(proxy)
    assert curve is not None
    value = np.array(list(range(10)), dtype="uint32")
    array_hash = Hash("type", 14,
                      "data", value.tobytes())
    h = Hash("prop", array_hash)
    set_proxy_hash(proxy, h)
    np.testing.assert_array_equal(curve.yData, value)

    # teardown
    controller.destroy()
    assert controller.widget is None
