from karabo.common.scenemodel.api import LineEditModel
from karabo.middlelayer import Configurable, String
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)
from ..lineedit import EditableLineEdit


class Object(Configurable):
    prop = String()


class TestEditableLineEdit(GuiTestCase):
    def setUp(self):
        super(TestEditableLineEdit, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.controller = EditableLineEdit(proxy=self.proxy,
                                           model=LineEditModel())
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        self.controller._last_cursor_pos = 0
        set_proxy_value(self.proxy, 'prop', 'Blah')
        assert self.controller.widget.text() == 'Blah'
        assert self.controller.widget.cursorPosition() == 0

    def test_edit_value(self):
        self.controller.widget.textChanged.emit('Wha??')
        assert self.proxy.edit_value == 'Wha??'
