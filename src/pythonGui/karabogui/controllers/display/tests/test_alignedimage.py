from PyQt4.QtGui import QSpinBox, QSlider

from karabogui.binding.api import (
    apply_configuration, build_binding, DeviceProxy, PropertyProxy)
from karabogui.testing import GuiTestCase
from .image import PipelineData, get_image_hash
from ..alignedimage import DisplayAlignedImage


class TestDisplayAlignedImage(GuiTestCase):
    def setUp(self):
        super(TestDisplayAlignedImage, self).setUp()

        schema = PipelineData.getClassSchema()
        binding = build_binding(schema)
        self.root_proxy = DeviceProxy(binding=binding)

    def test_2dimage(self):
        ouput_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                    path='output.data')
        img_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                  path='output.data.image')
        controller = DisplayAlignedImage(proxy=img_proxy)
        controller.create(None)

        # give qwt sometime to construct
        self.process_qt_events()

        assert controller.widget is not None
        assert controller._images[img_proxy] is None

        apply_configuration(get_image_hash(), ouput_proxy.binding)
        assert controller._images[img_proxy] is not None

        controller.destroy()

    def test_3dimage(self):
        ouput_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                    path='output.data')
        img_proxy = PropertyProxy(root_proxy=self.root_proxy,
                                  path='output.data.image')
        controller = DisplayAlignedImage(proxy=img_proxy)
        controller.create(None)

        # give qwt sometime to construct
        self.process_qt_events()
        assert isinstance(controller._currentCell, QSpinBox)
        assert isinstance(controller._slider, QSlider)

        apply_configuration(get_image_hash(dimz=True),
                            ouput_proxy.binding)
        img = controller._images[img_proxy][0]
        assert img is not None

        # new qwt image
        apply_configuration(get_image_hash(val=1, dimz=True),
                            ouput_proxy.binding)
        assert controller._images[img_proxy][0] is not img
        newimg = controller._images[img_proxy][0]

        # same qwt image, different data
        apply_configuration(get_image_hash(val=2, dimz=True, update=False),
                            ouput_proxy.binding)
        assert controller._images[img_proxy][0] is newimg

        controller._currentCell.setValue(0)
        assert controller._slider.sliderPosition() == 0

        for i in range(3):
            controller._axisChanged(i)
            controller._sliderMoved(1)
            assert controller._axis == i

        controller.destroy()
