from unittest.mock import patch

from PyQt4.QtGui import QDialog

from karabo.middlelayer import Configurable, VectorString
from karabo_gui.binding.api import apply_default_configuration
from karabo_gui.controllers.listedit import ListEdit
from karabo_gui.testing import GuiTestCase, get_class_property_proxy
from ..strlist import EditableListElement


class Object(Configurable):
    prop = VectorString(defaultValue=['hi there'])


class ListEditMock(ListEdit):
    @property
    def values(self):
        return ['foo', 'bar']

    def exec_(self):
        return QDialog.Accepted


class TestEditableListElement(GuiTestCase):
    def setUp(self):
        super(TestEditableListElement, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.controller = EditableListElement(proxy=self.proxy)
        self.controller.create(None)
        apply_default_configuration(self.proxy.root_proxy.binding)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_edit_dialog(self):
        target = 'karabo_gui.controllers.edit.strlist.ListEdit'
        with patch(target, new=ListEditMock):
            self.controller._on_edit_clicked()
            assert self.proxy.value == ['foo', 'bar']
