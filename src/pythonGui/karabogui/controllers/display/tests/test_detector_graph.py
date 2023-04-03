import pytest
from qtpy.QtWidgets import QGraphicsTextItem

from karabo.native import EncodingType
from karabogui.binding.builder import build_binding
from karabogui.binding.config import apply_configuration
from karabogui.binding.proxy import DeviceProxy, PropertyProxy
from karabogui.controllers.display.tests.image import (
    get_image_hash, get_pipeline_schema)
from karabogui.graph.common.api import Axes
from karabogui.testing import GuiTestCase
from karabogui.util import process_qt_events

from ..detector_graph import DisplayDetectorGraph, FrameSlider


class TestFrameSlider(GuiTestCase):

    def setUp(self):
        super(TestFrameSlider, self).setUp()

        self.frame_slider = FrameSlider()
        self.frame_slider.set_slider_maximum(10)
        self.frame_slider.axisChanged.connect(self._on_axisChanged)
        self.frame_slider.cellChanged.connect(self._on_cell_changed)

        self._axis = None
        self._cell = None

    def _on_axisChanged(self, value):
        self._axis = value

    def _on_cell_changed(self, value):
        self._cell = value

    def _set_axis_value(self, axis):
        """Sets the given value and emit the appropriate signal"""
        cb_axis = self.frame_slider.cb_axis
        cb_axis.setCurrentIndex(Axes[axis].value)
        cb_axis.currentIndexChanged['const QString &'].emit(axis)

    def _set_slider_value(self, value):
        """Sets the given slider position and emit the appropriate signal"""
        self.frame_slider.sl_cell.setSliderPosition(value)
        self.frame_slider.sl_cell.valueChanged.emit(value)

    def _set_spinbox_value(self, value):
        """Sets the given spinbox value and emit the appropriate signal"""
        self.frame_slider.sb_cell.setValue(value)
        self.frame_slider.sb_cell.valueChanged.emit(value)

    def test_combobox_axis(self):
        """Assert combobox contents"""
        cb_axis = self.frame_slider.cb_axis
        for axis in list(Axes):
            self.assertEqual(cb_axis.itemText(axis.value), axis.name)

    def test_signals(self):
        # Test combobox
        self.assertIsNone(self._axis)
        self._set_axis_value('Y')
        self.assertEqual(self._axis, 'Y')

        self._set_axis_value('X')
        self.assertEqual(self._axis, 'X')

        # Test slider
        self.assertIsNone(self._cell)
        self._set_slider_value(0)
        self.assertEqual(self._cell, 0)
        self.assertEqual(self.frame_slider.sb_cell.value(), 0)

        self._set_slider_value(7)
        self.assertEqual(self._cell, 7)
        self.assertEqual(self.frame_slider.sb_cell.value(), 7)

        # Test spinbox
        self._set_spinbox_value(0)
        self.assertEqual(self._cell, 0)
        self.assertEqual(self.frame_slider.sl_cell.value(), 0)

        self._set_spinbox_value(7)
        self.assertEqual(self._cell, 7)
        self.assertEqual(self.frame_slider.sl_cell.value(), 7)


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


@pytest.mark.skip(reason='Visibility tests are not working')
def test_2d_image(detectorGraphTest):
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
    assert frame_slider.isVisible()

    image_hash = get_image_hash()
    apply_configuration(image_hash, output_proxy.binding)

    assert frame_slider.isVisible()


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
    image_hash = get_image_hash(dimZ=3, encoding=EncodingType.YUV)
    apply_configuration(image_hash, output_proxy.binding)

    image_node = controller._image_node
    assert image_node.encoding is EncodingType.GRAY
    assert image_node.dim_x == 40
    assert image_node.dim_y == 30
    assert image_node.dim_z == 0
