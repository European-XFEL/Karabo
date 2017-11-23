from karabo.common.scenemodel.api import ComboBoxModel
from karabo.middlelayer import Configurable, String, Int32
from karabogui.binding.api import build_binding
from karabogui.testing import GuiTestCase, get_class_property_proxy
from ..combobox import EditableComboBox


class Object(Configurable):
    prop = String(options=['foo', 'bar', 'baz', 'qux'])


class Other(Configurable):
    prop = Int32(options=[1, 2, 3, 5, 8])


class TestEditableComboBox(GuiTestCase):
    def setUp(self):
        super(TestEditableComboBox, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.controller = EditableComboBox(proxy=self.proxy,
                                           model=ComboBoxModel())
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        self.proxy.value = 'bar'
        assert self.controller.widget.currentIndex() == 1

    def test_edit_value(self):
        self.controller.widget.setCurrentIndex(3)
        assert self.proxy.value == 'qux'

    def test_schema_update(self):
        proxy = get_class_property_proxy(Other.getClassSchema(), 'prop')
        controller = EditableComboBox(proxy=proxy)
        controller.create(None)

        assert controller.widget.count() == 5

        build_binding(Object.getClassSchema(),
                      existing=proxy.root_proxy.binding)

        assert controller.widget.count() == 4
