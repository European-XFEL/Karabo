from karabo.middlelayer import Configurable, Int8, String
from karabogui.binding.api import build_binding, apply_default_configuration
from karabogui.testing import GuiTestCase, get_class_property_proxy
from ..combobox import DisplayComboBox


class Object(Configurable):
    prop = String(options=['foo', 'bar', 'baz', 'qux'])


class Object2(Configurable):
    prop = Int8(options=[42])


class TestDisplayComboBox(GuiTestCase):
    def setUp(self):
        super(TestDisplayComboBox, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DisplayComboBox(proxy=self.proxy)
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_setup(self):
        assert self.controller.widget.count() == 4

    def test_set_value(self):
        apply_default_configuration(self.proxy.root_proxy.binding)
        assert self.controller.widget.currentText() == 'foo'

        self.proxy.binding.value = 'bar'
        assert self.controller.widget.currentText() == 'bar'

    def test_binding_change(self):
        try:
            schema = Object2.getClassSchema()
            build_binding(schema, existing=self.proxy.root_proxy.binding)
            assert self.controller.widget.count() == 1
        finally:
            # Put things back as they were!
            schema = Object.getClassSchema()
            build_binding(schema, existing=self.proxy.root_proxy.binding)
