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

from karabo.common.scenemodel.api import build_model_config
from karabo.native import Bool, Configurable, Int32
from karabogui.binding.api import PropertyProxy, build_binding
from karabogui.testing import (
    get_class_property_proxy, get_property_proxy, set_proxy_value)

from ..multicurvegraph import DisplayMultiCurveGraph


class Object(Configurable):
    reset = Bool()
    index = Int32()
    value = Int32()


@pytest.fixture
def multicurvegraph_setup(gui_app):
    schema = Object.getClassSchema()
    reset = get_class_property_proxy(schema, "reset")
    device = reset.root_proxy

    index = PropertyProxy(root_proxy=device, path="index")
    value = PropertyProxy(root_proxy=device, path="value")

    controller = DisplayMultiCurveGraph(proxy=index)
    controller.create(None)
    controller.visualize_additional_property(reset)
    controller.visualize_additional_property(value)

    yield controller, reset, index, value

    controller.destroy()
    assert controller.widget is None


def test_set_value(multicurvegraph_setup):
    controller, reset, index, value = multicurvegraph_setup
    assert len(controller._curves) == 1
    set_proxy_value(index, "index", 1)
    set_proxy_value(value, "value", 42)
    empty = controller._curves.get(index)
    assert empty is None
    curve = controller._curves.get(value)
    assert curve is not None
    assert list(curve.xData) == [1.0]
    assert list(curve.yData) == [42.0]
    set_proxy_value(index, "index", 2)
    set_proxy_value(value, "value", 37)
    assert list(curve.xData) == [1.0, 2.0]
    assert list(curve.yData) == [42.0, 37.0]

    set_proxy_value(reset, "reset", True)
    curve = controller._curves.get(value)
    assert curve is not None

    empty_x = curve.xData is None or list(curve.xData) == []
    empty_y = curve.yData is None or list(curve.yData) == []
    assert empty_x
    assert empty_y

    set_proxy_value(value, "value", 42)
    curve = controller._curves.get(value)

    empty = curve.yData is None or list(curve.yData) == []
    assert empty
    set_proxy_value(index, "index", 1)
    assert list(curve.yData) == [42.0]


def test_visualize_additional_nobinding(multicurvegraph_setup):
    controller, _, _, value = multicurvegraph_setup
    proxy = get_property_proxy(None, "value", "TestDevice2")
    assert controller.visualize_additional_property(proxy)
    assert len(controller._curves) == 1
    build_binding(Object.getClassSchema(),
                  existing=proxy.root_proxy.binding)
    assert len(controller._curves) == 2


def test_configuration(multicurvegraph_setup):
    controller, _, _, _ = multicurvegraph_setup
    default_config = build_model_config(controller.model)
    configuration = controller.widget.configuration
    assert configuration
    assert configuration == default_config


def test_remove_proxy(multicurvegraph_setup):
    controller, reset, _, value = multicurvegraph_setup
    assert controller.remove_additional_property(reset)
    assert controller._reset_proxy is None

    legend = controller.widget._legend
    assert legend.isVisible()

    assert controller._curves.get(value)
    assert controller.remove_additional_property(value)
    assert controller._curves.get(value) is None
    assert not legend.isVisible()
