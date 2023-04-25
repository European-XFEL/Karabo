# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest.mock import patch

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDialog

from karabo.native import Configurable, VectorString
from karabogui.binding.api import apply_default_configuration
from karabogui.const import IS_MAC_SYSTEM
from karabogui.dialogs.listedit import ListEditDialog
from karabogui.testing import GuiTestCase, get_class_property_proxy

from ..strlist import EditableListElement


class Object(Configurable):
    prop = VectorString(defaultValue=['hi there'])


class ListEditMock(ListEditDialog):
    @property
    def values(self):
        return ['foo', 'bar']

    def exec(self):
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

    def test_focus_policy(self):
        if IS_MAC_SYSTEM:
            assert self.controller.widget.focusPolicy() == Qt.TabFocus
        else:
            assert self.controller.widget.focusPolicy() == Qt.StrongFocus

    def test_edit_dialog(self):
        target = 'karabogui.controllers.edit.strlist.ListEditDialog'
        with patch(target, new=ListEditMock):
            self.controller._on_edit_clicked()
            assert self.proxy.edit_value == ['foo', 'bar']
