from karabo.native import Configurable, Int8, UInt8
from karabogui.binding.api import build_binding
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..hex import Hexadecimal


class Object(Configurable):
    prop = UInt8()


class Other(Configurable):
    prop = Int8(minInc=-1, maxInc=15)


class TestHexadecimal(GuiTestCase):
    def setUp(self):
        super(TestHexadecimal, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.controller = Hexadecimal(proxy=self.proxy)
        self.controller.create(None)
        self.controller.set_read_only(False)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', 15)
        assert self.controller._internal_widget.text() == 'f'

    def test_edit_value(self):
        self.controller._internal_widget.setText('ff')
        assert self.proxy.edit_value == 255

    def test_schema_update(self):
        proxy = get_class_property_proxy(Other.getClassSchema(), 'prop')
        controller = Hexadecimal(proxy=proxy)
        controller.create(None)

        assert controller._internal_widget.inputMask() == '#h; '

        build_binding(Object.getClassSchema(),
                      existing=proxy.root_proxy.binding)

        assert controller._internal_widget.inputMask() == 'hh; '
