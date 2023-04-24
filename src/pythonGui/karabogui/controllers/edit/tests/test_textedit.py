# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from qtpy.QtCore import Qt

from karabo.common.scenemodel.api import LineEditModel
from karabo.native import Char, Configurable, String
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..textedit import EditableLineEdit


class Object(Configurable):
    prop = String()


class ObjectChar(Configurable):
    prop = Char()


class TestEditableLineEdit(GuiTestCase):
    def setUp(self):
        super(TestEditableLineEdit, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.controller = EditableLineEdit(proxy=self.proxy,
                                           model=LineEditModel())
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_focus_policy(self):
        assert self.controller.widget.focusPolicy() == Qt.StrongFocus

    def test_set_value(self):
        self.controller._last_cursor_pos = 0
        set_proxy_value(self.proxy, 'prop', 'Blah')
        self.assertEqual(self.controller.widget.text(), 'Blah')
        self.assertEqual(self.controller.widget.cursorPosition(), 0)

    def test_edit_value(self):
        self.controller.widget.textChanged.emit('Wha??')
        self.assertEqual(self.proxy.edit_value, 'Wha??')


class TestCharEditableLineEdit(GuiTestCase):
    def setUp(self):
        super(TestCharEditableLineEdit, self).setUp()
        self.proxy = get_class_property_proxy(ObjectChar.getClassSchema(),
                                              'prop')
        self.controller = EditableLineEdit(proxy=self.proxy,
                                           model=LineEditModel())
        self.controller.create(None)

    def tearDown(self):
        super(TestCharEditableLineEdit, self).tearDown()
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)

    def test_set_value(self):
        self.controller._last_cursor_pos = 0
        set_proxy_value(self.proxy, 'prop', 'D')
        self.assertEqual(self.controller.widget.text(), 'D')
        self.assertEqual(self.controller.widget.cursorPosition(), 0)

    def test_edit_value(self):
        set_proxy_value(self.proxy, 'prop', '$')
        self.assertEqual(self.controller.widget.text(), '$')
        self.controller.widget.setText('12345')
        # stop at the first char
        self.assertEqual(self.proxy.edit_value, '1')
