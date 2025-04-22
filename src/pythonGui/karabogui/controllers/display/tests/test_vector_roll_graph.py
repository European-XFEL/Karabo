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
from platform import system

import numpy as np
import pytest
from qtpy.QtWidgets import QGraphicsTextItem

from karabo.common.scenemodel.api import VectorRollGraphModel
from karabo.native import (
    Configurable, Hash, Int32, NDArray, Timestamp, VectorInt32)
from karabogui.graph.common.api import AuxPlots
from karabogui.testing import (
    get_class_property_proxy, set_proxy_hash, set_proxy_value)

from ..vector_roll_graph import ArrayRollGraph


class Object(Configurable):
    prop = VectorInt32(defaultValue=[1, 2, 3])


class NDArrayObject(Configurable):
    prop = NDArray(
        defaultValue=np.arange(10, dtype=np.int32),
        shape=(10,),
        dtype=Int32)


@pytest.fixture
def vector_roll_graph_setup(gui_app, mocker):
    schema = Object.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    controller = ArrayRollGraph(proxy=proxy, model=VectorRollGraphModel())

    mocker.patch.object(QGraphicsTextItem, "setHtml")
    controller.create(None)
    assert controller.widget is not None

    yield controller, proxy

    controller.destroy()
    assert controller.widget is None


def test_vector_roll_graph_configuration(vector_roll_graph_setup):
    """Assert that the vector roll auxiliar plots do not smooth the
    images"""
    controller, _ = vector_roll_graph_setup
    aux_plots = controller.widget._aux_plots
    controller = aux_plots._aggregators[AuxPlots.ProfilePlot]
    assert not controller.smooth


@pytest.mark.skipif(system() == "Windows",
                    reason="image.data is None in Windows tests")
def test_vector_roll_graph_set_value(vector_roll_graph_setup):
    """Test the value setting in VectorRollGraph"""
    controller, proxy = vector_roll_graph_setup
    plot = controller._plot
    assert plot is not None
    value = [2, 4, 6]
    set_proxy_value(proxy, "prop", value)
    image = controller._image
    np.testing.assert_almost_equal(image.data[0], value)


@pytest.mark.skipif(system() == "Windows",
                    reason="image.data is None in Windows tests")
def test_vector_roll_graph_set_value_timestamp(vector_roll_graph_setup):
    """Test the value setting with same timestamp in VectorRollGraph"""
    controller, proxy = vector_roll_graph_setup
    plot = controller._plot
    assert plot is not None
    timestamp = Timestamp()
    h = Hash("prop", [2, 4, 6])
    set_proxy_hash(proxy, h, timestamp)
    image = controller._image
    np.testing.assert_almost_equal(image.data[0], [2, 4, 6])
    h = Hash("prop", [22, 34, 16])
    set_proxy_hash(proxy, h, timestamp)
    image = controller._image
    np.testing.assert_almost_equal(image.data[0], [2, 4, 6])


def test_vector_roll_graph_image_stack_configuration(vector_roll_graph_setup,
                                                     mocker):
    _, proxy = vector_roll_graph_setup
    controller = ArrayRollGraph(proxy=proxy, model=VectorRollGraphModel())
    mocker.patch.object(QGraphicsTextItem, "setHtml")
    controller.create(None)

    action = controller.widget.actions()[4]
    assert action.text() == "Image Size"

    dsym = "karabogui.controllers.display.vector_roll_graph.QInputDialog"
    QInputDialog = mocker.patch(dsym)
    QInputDialog.getInt.return_value = 20, True
    action.trigger()
    assert controller.model.maxlen == 20
    assert controller._image.stack == 20
    assert controller._image.data is None

    controller.destroy()


@pytest.mark.skipif(system() == "Windows",
                    reason="image.data is None in Windows tests")
def test_set_value(gui_app):
    # setup
    schema = NDArrayObject.getClassSchema()
    proxy = get_class_property_proxy(schema, "prop")
    controller = ArrayRollGraph(proxy=proxy)
    controller.create(None)
    assert controller.widget is not None

    # test body
    value = np.array(
        [2, 3, 2, 3, 2, 3, 2, 3, 2, 3],
        dtype=np.int32)
    array_hash = Hash("type", 12,
                      "data", value.tobytes())
    set_proxy_value(proxy, "prop", array_hash)

    image = controller._image
    np.testing.assert_almost_equal(image.data[0], value)

    # teardown
    controller.destroy()
    assert controller.widget is None
