import numpy as np
from PyQt4.QtCore import Qt

from karabo.native import EncodingType
from karabogui.binding.api import (
    apply_configuration, build_binding, DeviceProxy, PropertyProxy)
from karabogui.testing import GuiTestCase
from .image import PipelineData, get_image_hash, dimX, dimY
from ..imageview import DisplayImage


class TestDisplayImage(GuiTestCase):
    def setUp(self):
        super(TestDisplayImage, self).setUp()

        schema = PipelineData.getClassSchema()
        binding = build_binding(schema)
        self.root_proxy = DeviceProxy(binding=binding)

    def test_2dimage(self):
        output_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                     path='output.data')
        img_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                  path='output.data.image')
        controller = DisplayImage(proxy=img_proxy)
        controller.create(None)

        # give qwt sometime to construct
        self.process_qt_events()

        assert controller.widget is not None
        assert controller._plot is not None

        apply_configuration(get_image_hash(), output_proxy.binding)
        img_arr = np.array(controller._img_array)
        assert np.all(img_arr == np.zeros((dimY, dimX)))

        # parasitically walk some code path
        controller._cbUpdateColorMap.setCheckState(Qt.Checked)

        apply_configuration(get_image_hash(val=1), output_proxy.binding)
        img_arr = np.array(controller._img_array)
        assert np.all(img_arr == np.ones((dimY, dimX)))
        controller.destroy()

    def test_rgbimage(self):
        output_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                     path='output.data')
        img_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                  path='output.data.image')
        controller = DisplayImage(proxy=img_proxy)
        controller.create(None)

        # give qwt sometime to construct
        self.process_qt_events()

        assert controller.widget is not None
        assert controller._plot is not None

        apply_configuration(
            get_image_hash(dimZ=3, encoding=EncodingType.RGB),
            output_proxy.binding)
        img_arr = np.array(controller._img_array)
        assert np.all(img_arr == np.zeros((dimY, dimX, 3)))
        controller.destroy()

    def test_yuv422image(self):
        output_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                     path='output.data')
        img_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                  path='output.data.image')
        controller = DisplayImage(proxy=img_proxy)
        controller.create(None)

        # give qwt sometime to construct
        self.process_qt_events()

        assert controller.widget is not None
        assert controller._plot is not None

        apply_configuration(
            get_image_hash(dimZ=2, encoding=EncodingType.YUV),
            output_proxy.binding)
        img_arr = np.array(controller._img_array)
        assert np.all(img_arr == np.zeros((dimY, dimX)))
        controller.destroy()

    def test_yuv444image(self):
        output_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                     path='output.data')
        img_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                  path='output.data.image')
        controller = DisplayImage(proxy=img_proxy)
        controller.create(None)

        # give qwt sometime to construct
        self.process_qt_events()

        assert controller.widget is not None
        assert controller._plot is not None

        apply_configuration(
            get_image_hash(dimZ=3, encoding=EncodingType.YUV),
            output_proxy.binding)
        img_arr = np.array(controller._img_array)
        assert np.all(img_arr == np.zeros((dimY, dimX)))
        controller.destroy()

    def test_yuvother_image(self):
        output_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                     path='output.data')
        img_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                  path='output.data.image')
        controller = DisplayImage(proxy=img_proxy)
        controller.create(None)

        # give qwt sometime to construct
        self.process_qt_events()

        assert controller.widget is not None
        assert controller._plot is not None

        apply_configuration(
            get_image_hash(dimZ=1, encoding=EncodingType.YUV),
            output_proxy.binding)
        img_arr = np.array(controller._img_array)
        assert np.all(img_arr.size == 0)  # should bail
        controller.destroy()

    def test_3dimage(self):
        output_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                     path='output.data')
        img_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                  path='output.data.image')
        controller = DisplayImage(proxy=img_proxy)
        controller.create(None)

        # give qwt sometime to construct
        self.process_qt_events()

        assert controller.widget is not None
        assert controller._plot is not None

        apply_configuration(get_image_hash(dimZ=2), output_proxy.binding)
        img_arr = np.array(controller._img_array)
        assert np.all(img_arr == np.zeros((dimY, dimX, 2)))

        # parasitically walk some code path
        controller._cell_changed(0)
        for i in range(3):
            controller._axis_changed(i)
        controller._unset_slider()
