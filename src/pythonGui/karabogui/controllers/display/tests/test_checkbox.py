from karabo.native import Bool, Configurable
from karabogui.testing import (
    GuiTestCase, check_renderer_against_svg, get_class_property_proxy,
    set_proxy_value)

from ..checkbox import CHECKED, UNCHECKED, DisplayCheckBox


class Object(Configurable):
    prop = Bool(defaultValue=True)


class TestDisplayCheckBox(GuiTestCase):
    def setUp(self):
        super(TestDisplayCheckBox, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DisplayCheckBox(proxy=self.proxy)
        self.controller.create(None)

    def tearDown(self):
        super(TestDisplayCheckBox, self).tearDown()
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', True)
        check_renderer_against_svg(self.controller.widget.renderer(),
                                   CHECKED)

        set_proxy_value(self.proxy, 'prop', False)
        check_renderer_against_svg(self.controller.widget.renderer(),
                                   UNCHECKED)
