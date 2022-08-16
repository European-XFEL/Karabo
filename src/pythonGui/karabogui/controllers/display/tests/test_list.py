from qtpy.QtCore import Qt

from karabo.common.scenemodel.api import DisplayListModel
from karabo.native import Configurable, VectorInt32
from karabogui.binding.api import apply_default_configuration
from karabogui.controllers.display.list import DisplayList
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)


class Object(Configurable):
    prop = VectorInt32(defaultValue=[1])


class TestDisplayList(GuiTestCase):
    def setUp(self):
        super().setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
        self.controller = DisplayList(proxy=self.proxy,
                                      model=DisplayListModel())
        self.controller.create(None)
        apply_default_configuration(self.proxy.root_proxy.binding)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_focus_policy(self):
        assert self.controller.widget.focusPolicy() == Qt.NoFocus

    def test_set_value(self):
        set_proxy_value(self.proxy, "prop", [0, 2])
        assert self.controller.widget.text() == "0,2"
