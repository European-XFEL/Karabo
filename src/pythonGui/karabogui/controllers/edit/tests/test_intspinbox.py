from karabo.common.scenemodel.api import EditableSpinBoxModel
from karabo.native import Configurable, Int32, UInt8
from karabogui.binding.api import build_binding
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..intspinbox import EditableSpinBox


class Object(Configurable):
    prop = Int32(minInc=-10, maxInc=10)


class Other(Configurable):
    prop = UInt8()


class TestEditableSpinBox(GuiTestCase):
    def setUp(self):
        super(TestEditableSpinBox, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.controller = EditableSpinBox(proxy=self.proxy,
                                          model=EditableSpinBoxModel())
        self.controller.create(None)
        self.controller.set_read_only(False)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', 5)
        assert self.controller._internal_widget.value() == 5

    def test_edit_value(self):
        self.controller._internal_widget.setValue(3)
        assert self.proxy.edit_value == 3

    def test_schema_update(self):
        proxy = get_class_property_proxy(Other.getClassSchema(), 'prop')
        controller = EditableSpinBox(proxy=proxy)
        controller.create(None)

        assert controller._internal_widget.minimum() == 0
        assert controller._internal_widget.maximum() == 255

        build_binding(Object.getClassSchema(),
                      existing=proxy.root_proxy.binding)

        assert controller._internal_widget.minimum() == -10
        assert controller._internal_widget.maximum() == 10
