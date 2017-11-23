from unittest.mock import patch

from PyQt4.QtGui import QDialog

from karabo.common.scenemodel.api import EditableListModel
from karabo.middlelayer import Configurable, VectorInt32
from karabogui.binding.api import apply_default_configuration
from karabogui.controllers.listedit import ListEdit
from karabogui.testing import GuiTestCase, get_class_property_proxy
from ..list import EditableList


class Object(Configurable):
    prop = VectorInt32(defaultValue=[1])


class ListEditMock(ListEdit):
    @property
    def values(self):
        return ['-1', '42', '-1']

    def exec_(self):
        return QDialog.Accepted


class TestEditableList(GuiTestCase):
    def setUp(self):
        super(TestEditableList, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.controller = EditableList(proxy=self.proxy,
                                       model=EditableListModel())
        self.controller.create(None)
        apply_default_configuration(self.proxy.root_proxy.binding)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        self.controller.last_cursor_position = 0
        self.proxy.value = [0, 2]
        assert self.controller._internal_widget.text() == '0,2'
        assert self.controller._internal_widget.cursorPosition() == 0

    def test_edit_value(self):
        self.controller.set_read_only(False)
        self.controller._internal_widget.setText('3,4')
        assert all(self.proxy.value == [3, 4])

    def test_edit_empty_value(self):
        self.controller.set_read_only(False)
        self.controller._internal_widget.setText('')
        value = self.proxy.value
        assert len(value) == 0 and all(value == [])

    def test_no_edit_value(self):
        self.controller.set_read_only(True)
        self.controller._internal_widget.setText('3,4')

        value = self.proxy.value
        assert len(value) == 1 and all(value == [1])

    def test_edit_dialog(self):
        target = 'karabogui.controllers.edit.list.ListEdit'
        with patch(target, new=ListEditMock):
            self.controller.set_read_only(False)
            self.controller._on_edit_clicked()
            assert all(self.proxy.value == [-1, 42, -1])
