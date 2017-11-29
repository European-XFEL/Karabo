from karabogui.binding.api import (
    apply_configuration, build_binding, DeviceProxy, PropertyProxy)
from karabogui.testing import GuiTestCase
from .image import PipelineData, get_image_hash
from ..imageelement import DisplayImageElement


class TestDisplayImageElement(GuiTestCase):
    def setUp(self):
        super(TestDisplayImageElement, self).setUp()

        schema = PipelineData.getClassSchema()
        binding = build_binding(schema)
        root_proxy = DeviceProxy(binding=binding)

        self.output_proxy = PropertyProxy(root_proxy=root_proxy,
                                          path='output.data')
        self.img_proxy = PropertyProxy(root_proxy=root_proxy,
                                       path='output.data.image')
        self.controller = DisplayImageElement(proxy=self.img_proxy)

    def test_basics(self):
        self.controller.create(None)
        assert self.controller.widget is not None
        self.controller.destroy()
        assert self.controller.widget is None

    def test_2dimage(self):
        self.controller.create(None)
        apply_configuration(get_image_hash(), self.output_proxy.binding)
        assert self.controller.widget.pixmap() is not None

    def test_rgbimage(self):
        self.controller.create(None)
        apply_configuration(get_image_hash(dimz=True, rgb=True),
                            self.output_proxy.binding)
        assert self.controller.widget.pixmap() is not None

    def test_3dimage(self):
        self.controller.create(None)
        apply_configuration(get_image_hash(dimz=True),
                            self.output_proxy.binding)
        # should bail
        assert self.controller.widget.pixmap() is None
