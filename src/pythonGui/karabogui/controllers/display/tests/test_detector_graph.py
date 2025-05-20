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
from qtpy.QtWidgets import QGraphicsTextItem

from karabo.native import EncodingType
from karabogui.binding.builder import build_binding
from karabogui.binding.config import apply_configuration
from karabogui.binding.proxy import DeviceProxy, PropertyProxy
from karabogui.controllers.display.tests.image import (
    get_image_hash, get_pipeline_schema)
from karabogui.graph.common.api import Axes
from karabogui.util import process_qt_events

from ..detector_graph import DisplayDetectorGraph, FrameSlider


# class to make available slot for signal test
class DetectorGraphTestSlot:
    def __init__(self):
        self.value = None

    def on_value_changed(self, value):
        self.value = value


def test_detector_graph_signals(gui_app):
    # set up
    axis = DetectorGraphTestSlot()
    cell = DetectorGraphTestSlot()

    frame_slider = FrameSlider()
    frame_slider.set_slider_maximum(10)
    frame_slider.axisChanged.connect(axis.on_value_changed)
    frame_slider.cellChanged.connect(cell.on_value_changed)

    # Test combobox
    assert axis.value is None
    cb_axis = frame_slider.cb_axis
    cb_axis.setCurrentIndex(Axes['Y'].value)
    cb_axis.currentIndexChanged['const QString &'].emit(axis.value)
    assert axis.value == 'Y'

    cb_axis.setCurrentIndex(Axes['X'].value)
    cb_axis.currentIndexChanged['const QString &'].emit(axis.value)
    assert axis.value == 'X'

    # Test slider
    assert cell.value is None
    frame_slider.sl_cell.setSliderPosition(0)
    frame_slider.sl_cell.valueChanged.emit(0)
    assert cell.value == 0
    assert frame_slider.sb_cell.value() == 0

    frame_slider.sl_cell.setSliderPosition(7)
    frame_slider.sl_cell.valueChanged.emit(7)
    assert cell.value == 7
    assert frame_slider.sb_cell.value() == 7

    # Test spinbox
    frame_slider.sb_cell.setValue(0)
    frame_slider.sb_cell.valueChanged.emit(0)
    assert cell.value == 0
    assert frame_slider.sl_cell.value() == 0

    frame_slider.sb_cell.setValue(7)
    frame_slider.sb_cell.valueChanged.emit(7)
    assert cell.value == 7
    assert frame_slider.sl_cell.value() == 7


@pytest.fixture
def detectorGraphTest(gui_app, mocker):
    schema = get_pipeline_schema()
    binding = build_binding(schema)
    root_proxy = DeviceProxy(binding=binding)

    output_proxy = PropertyProxy(root_proxy=root_proxy,
                                 path='output.data')
    img_proxy = PropertyProxy(root_proxy=root_proxy,
                              path='output.data.image')
    controller = DisplayDetectorGraph(proxy=img_proxy)

    mocker.patch.object(QGraphicsTextItem, 'setHtml')
    controller.create(None)
    yield controller, output_proxy
    controller.destroy()


def test_detector_graph_2d_image(detectorGraphTest):
    """When a 2D image is supplied, the detector shouldn't show the
    frameslider"""
    controller, output_proxy = detectorGraphTest
    frame_slider = controller._frame_slider

    # Assert initial state
    assert not frame_slider.isVisible()

    image_hash = get_image_hash(dimZ=1)
    apply_configuration(image_hash, output_proxy.binding)

    process_qt_events()
    process_qt_events()
    # assert frame_slider.isVisible()

    image_hash = get_image_hash()
    apply_configuration(image_hash, output_proxy.binding)

    # assert frame_slider.isVisible()
    assert controller._plot.imageItem.image.shape == (30, 40)
    controller.clear_widget()
    assert controller._plot.imageItem.image.shape == (10, 10)


def test_detector_graph_basics(detectorGraphTest):
    controller, output_proxy = detectorGraphTest
    frame_slider = controller._frame_slider

    # Assert initial state
    assert not frame_slider.isVisible()
    assert controller._axis == Axes.Z
    assert controller._cell == 0

    image_hash = get_image_hash(dimZ=5)
    apply_configuration(image_hash, output_proxy.binding)

    # Viewing through Z axis
    image = controller.widget.plot().imageItem.image
    assert image.shape == (30, 40)
    assert frame_slider.sb_cell.maximum() == 4
    assert frame_slider.sb_cell.value() == 0

    # Let's modify the controller state through the frame slider
    frame_slider.sb_cell.valueChanged.emit(4)
    assert controller._cell == 4

    # Change axis, this should change the viewed image
    frame_slider.axisChanged.emit('Y')
    image = controller.widget.plot().imageItem.image
    assert image.shape == (40, 5)
    assert frame_slider.sb_cell.maximum() == 29
    assert frame_slider.sb_cell.value() == 0


def test_detector_graph_yuv_image(detectorGraphTest):
    controller, output_proxy = detectorGraphTest
    image_hash = get_image_hash(dimZ=3, encoding=EncodingType.YUV444)
    apply_configuration(image_hash, output_proxy.binding)

    image_node = controller._image_node
    assert image_node.encoding is EncodingType.GRAY
    assert image_node.dim_x == 40
    assert image_node.dim_y == 30
    assert image_node.dim_z == 0
