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

from karabo.common.scenemodel.api import VectorBarGraphModel
from karabo.native import Configurable, Hash, Int32, NDArray, VectorInt32
from karabogui.testing import get_class_property_proxy, set_proxy_value

from ..vector_bar_graph import DisplayVectorBarGraph


class Object(Configurable):
    value = VectorInt32()


class NDArrayObject(Configurable):
    value = NDArray(
        defaultValue=np.arange(10),
        shape=(10,),
        dtype=Int32,
    )


@pytest.fixture
def vector_bar_graph_setup(gui_app):
    # setup
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "value")
    controller = DisplayVectorBarGraph(proxy=proxy,
                                       model=VectorBarGraphModel())
    controller.create(None)
    assert controller.widget is not None
    yield controller, proxy
    # teardown
    controller.destroy()
    assert controller.widget is None


def test_bar_graph_basics(vector_bar_graph_setup):
    controller, proxy = vector_bar_graph_setup
    value = [2, 4, 6]
    set_proxy_value(proxy, "value", value)
    curve = controller._plot
    assert list(curve.opts.get("x")) == [0, 1, 2]
    assert list(curve.opts.get("height")) == value


def test_bar_graph_width(vector_bar_graph_setup, mocker):
    controller, proxy = vector_bar_graph_setup
    action = controller.widget.actions()[9]
    assert action.text() == "Bar Width"

    dsym = "karabogui.controllers.display.vector_bar_graph.QInputDialog"
    QInputDialog = mocker.patch(dsym)
    QInputDialog.getDouble.return_value = 2.7, True
    action.trigger()
    assert controller.model.bar_width == 2.7
    assert controller._plot.opts["width"] == 2.7


def test_array_bar_graph_basics(gui_app):
    # setup
    schema = NDArrayObject.getClassSchema()
    proxy = get_class_property_proxy(schema, "value")
    controller = DisplayVectorBarGraph(proxy=proxy)
    controller.create(None)
    assert controller.widget is not None

    # test body
    value = np.array(
        [2, 3, 2, 3, 2, 3, 2, 3, 2, 3],
        dtype=np.int32)
    array_hash = Hash("type", 12,
                      "data", value.tobytes())
    set_proxy_value(proxy, "value", array_hash)
    curve = controller._plot
    np.testing.assert_array_equal(
        list(curve.opts.get("x")), np.arange(10))
    np.testing.assert_array_equal(
        list(curve.opts.get("height")), value)

    # teardown
    controller.destroy()
    assert controller.widget is None
