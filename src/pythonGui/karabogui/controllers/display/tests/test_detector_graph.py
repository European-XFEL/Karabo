from qtpy.QtWidgets import QGraphicsTextItem
from unittest import mock, skip

from karabo.native import EncodingType

from karabogui.graph.common.api import Axes
from karabogui.binding.builder import build_binding
from karabogui.binding.config import apply_configuration
from karabogui.binding.proxy import DeviceProxy, PropertyProxy
from karabogui.controllers.display.tests.image import (
    get_image_hash, get_pipeline_schema)
from karabogui.testing import GuiTestCase

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


class TestDetectorGraph(GuiTestCase):

    def setUp(self):
        super(TestDetectorGraph, self).setUp()

        schema = get_pipeline_schema()
        binding = build_binding(schema)
        root_proxy = DeviceProxy(binding=binding)

        self.output_proxy = PropertyProxy(root_proxy=root_proxy,
                                          path='output.data')
        self.img_proxy = PropertyProxy(root_proxy=root_proxy,
                                       path='output.data.image')
        self.controller = DisplayDetectorGraph(proxy=self.img_proxy)

        with mock.patch.object(QGraphicsTextItem, 'setHtml'):
            self.controller.create(None)

    def tearDown(self):
        super(TestDetectorGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def _assert_image_shape(self, shape):
        image = self.controller.widget.plot().imageItem.image
        self.assertEqual(image.shape, shape)

    @skip(reason='Visibility tests are not working')
    def test_2d_image(self):
        """When a 2D image is supplied, the detector shouldn't show the
        frameslider"""
        frame_slider = self.controller._frame_slider

        # Assert initial state
        self.assertFalse(frame_slider.isVisible())

        image_hash = get_image_hash(dimZ=1)
        apply_configuration(image_hash, self.output_proxy.binding)

        self.process_qt_events()
        self.process_qt_events()
        self.assertTrue(frame_slider.isVisible())

        image_hash = get_image_hash()
        apply_configuration(image_hash, self.output_proxy.binding)

        self.assertTrue(frame_slider.isVisible())

    def test_basics(self):
        frame_slider = self.controller._frame_slider

        # Assert initial state
        self.assertFalse(frame_slider.isVisible())
        self.assertEqual(self.controller._axis, Axes.Z)
        self.assertEqual(self.controller._cell, 0)

        image_hash = get_image_hash(dimZ=5)
        apply_configuration(image_hash, self.output_proxy.binding)

        # Viewing through Z axis
        self._assert_image_shape((30, 40))
        self.assertEqual(frame_slider.sb_cell.maximum(), 4)
        self.assertEqual(frame_slider.sb_cell.value(), 0)

        # Let's modify the controller state through the frame slider
        frame_slider.sb_cell.valueChanged.emit(4)
        self.assertEqual(self.controller._cell, 4)

        # Change axis, this should change the viewed image
        frame_slider.axisChanged.emit('Y')
        self._assert_image_shape((40, 5))
        self.assertEqual(frame_slider.sb_cell.maximum(), 29)
        self.assertEqual(frame_slider.sb_cell.value(), 0)

    def test_yuv_image(self):
        image_hash = get_image_hash(dimZ=3, encoding=EncodingType.YUV)
        apply_configuration(image_hash, self.output_proxy.binding)

        image_node = self.controller._image_node
        self.assertEqual(image_node.encoding, EncodingType.GRAY)
        self.assertEqual(image_node.dim_x, 40)
        self.assertEqual(image_node.dim_y, 30)
        self.assertEqual(image_node.dim_z, 0)
