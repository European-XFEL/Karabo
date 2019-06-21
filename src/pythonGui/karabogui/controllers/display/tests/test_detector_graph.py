from PyQt4 import QtGui
from unittest import mock

from karabo.native import EncodingType

from karabogui.binding.builder import build_binding
from karabogui.binding.config import apply_configuration
from karabogui.binding.proxy import DeviceProxy, PropertyProxy
from karabogui.controllers.display.tests.image import (
    PipelineData, get_image_hash)
from karabogui.testing import GuiTestCase

from ..display_detector_graph import DisplayDetectorGraph, FrameSlider


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

    def _set_axis_value(self, value):
        """Sets the given value and emit the appropriate signal"""
        self.frame_slider.ui_axis.setCurrentIndex(value)
        self.frame_slider.ui_axis.currentIndexChanged.emit(value)

    def _set_slider_value(self, value):
        """Sets the given slider position and emit the appropriate signal"""
        self.frame_slider.ui_slider_cell.setSliderPosition(value)
        self.frame_slider.ui_slider_cell.valueChanged.emit(value)

    def _set_spinbox_value(self, value):
        """Sets the given spinbox value and emit the appropriate signal"""
        self.frame_slider.ui_cell.setValue(value)
        self.frame_slider.ui_cell.valueChanged.emit(value)

    def test_combobox_axis(self):
        contents = [self.frame_slider.ui_axis.itemText(i) for i in
                    range(self.frame_slider.ui_axis.count())]
        self.assertListEqual(contents, ['0', '1', '2'])

    def test_signals(self):
        # Test combobox
        self.assertIsNone(self._axis)
        self._set_axis_value(0)
        self.assertEqual(self._axis, 0)

        self._set_axis_value(1)
        self.assertEqual(self._axis, 1)

        # Test slider
        self.assertIsNone(self._cell)
        self._set_slider_value(0)
        self.assertEqual(self._cell, 0)
        self.assertEqual(self.frame_slider.ui_cell.value(), 0)

        self._set_slider_value(7)
        self.assertEqual(self._cell, 7)
        self.assertEqual(self.frame_slider.ui_cell.value(), 7)

        # Test spinbox
        self._set_spinbox_value(0)
        self.assertEqual(self._cell, 0)
        self.assertEqual(self.frame_slider.ui_slider_cell.value(), 0)

        self._set_spinbox_value(7)
        self.assertEqual(self._cell, 7)
        self.assertEqual(self.frame_slider.ui_slider_cell.value(), 7)


class TestDetectorGraph(GuiTestCase):

    def setUp(self):
        super(TestDetectorGraph, self).setUp()

        schema = PipelineData.getClassSchema()
        binding = build_binding(schema)
        root_proxy = DeviceProxy(binding=binding)

        self.output_proxy = PropertyProxy(root_proxy=root_proxy,
                                          path='output.data')
        self.img_proxy = PropertyProxy(root_proxy=root_proxy,
                                       path='output.data.image')
        self.controller = DisplayDetectorGraph(proxy=self.img_proxy)

        with mock.patch.object(QtGui.QGraphicsTextItem, 'setHtml'):
            self.controller.create(None)

    def tearDown(self):
        super(TestDetectorGraph, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_basics(self):
        image_hash = get_image_hash(dimZ=3, encoding=EncodingType.YUV)

        apply_configuration(image_hash, self.output_proxy.binding)

        # Break encapsulation but oh well
        image_node = self.controller._image_node
        self.assertEqual(image_node.encoding, EncodingType.GRAY)
        self.assertEqual(image_node.dim_x, 40)
        self.assertEqual(image_node.dim_y, 30)
        self.assertEqual(image_node.dim_z, 0)
