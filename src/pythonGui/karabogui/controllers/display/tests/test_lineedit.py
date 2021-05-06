from karabo.native import Configurable, String
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..lineedit import DisplayLineEdit


class Object(Configurable):
    prop = String()


class TestDisplayLineEdit(GuiTestCase):
    def setUp(self):
        super(TestDisplayLineEdit, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')

    def test_basics(self):
        controller = DisplayLineEdit(proxy=self.proxy)
        controller.create(None)
        assert controller.widget is not None

        controller.destroy()
        assert controller.widget is None

    def test_set_string_value(self):
        controller = DisplayLineEdit(proxy=self.proxy)
        controller.create(None)
        set_proxy_value(self.proxy, 'prop', 'hello')
        assert controller.widget.text() == 'hello'
