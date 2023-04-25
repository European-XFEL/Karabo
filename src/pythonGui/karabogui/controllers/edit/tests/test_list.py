# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest.mock import patch

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import (
    EditableListModel, EditableRegexListModel)
from karabo.native import (
    Configurable, VectorInt8, VectorInt32, VectorRegexString)
from karabogui.binding.api import apply_default_configuration, get_min_max_size
from karabogui.dialogs.listedit import ListEditDialog
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..list import EditableList, EditableRegexList


class Object(Configurable):
    prop = VectorInt32(defaultValue=[1])


class SizeObject(Configurable):
    prop = VectorInt32(defaultValue=[1], minSize=1, maxSize=3)


class ShortObject(Configurable):
    prop = VectorInt8(defaultValue=[1])


class RegexObject(Configurable):
    prop = VectorRegexString(
        defaultValue=["remote:output"],
        regex=r"^[A-Za-z0-9_-]{1,60}(:)[A-Za-z0-9_-]{1,60}$")


class ListEditMock(ListEditDialog):
    @property
    def values(self):
        return ['-1', '42', '-1']

    def exec(self):
        return QDialog.Accepted


class TestEditableList(GuiTestCase):
    def setUp(self):
        super(TestEditableList, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.controller = EditableList(proxy=self.proxy,
                                       model=EditableListModel())
        self.controller.create(None)
        apply_default_configuration(self.proxy.root_proxy.binding)

        self.size_proxy = get_class_property_proxy(SizeObject.getClassSchema(),
                                                   'prop')
        self.size_controller = EditableList(proxy=self.size_proxy,
                                            model=EditableListModel())
        self.size_controller.create(None)
        apply_default_configuration(self.size_proxy.root_proxy.binding)

        self.short_proxy = get_class_property_proxy(
            ShortObject.getClassSchema(), 'prop')
        self.short_controller = EditableList(proxy=self.short_proxy,
                                             model=EditableListModel())
        self.short_controller.create(None)
        apply_default_configuration(self.short_proxy.root_proxy.binding)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_focus_policy(self):
        assert self.controller._internal_widget.focusPolicy() == Qt.StrongFocus

    def test_set_value(self):
        self.controller.last_cursor_position = 0
        set_proxy_value(self.proxy, 'prop', [0, 2])
        assert self.controller._internal_widget.text() == '0,2'
        assert self.controller._internal_widget.cursorPosition() == 0

    def test_edit_value(self):
        self.controller.set_read_only(False)
        self.controller._internal_widget.setText('3,4')
        assert all(self.proxy.edit_value == [3, 4])

    def test_edit_empty_value(self):
        self.controller._internal_widget.setText('NOT IMPORTANT')
        self.controller.set_read_only(False)
        self.controller._internal_widget.setText('')
        value = self.proxy.edit_value
        assert len(value) == 0 and all(value == [])

    def test_edit_dialog(self):
        target = 'karabogui.controllers.edit.list.ListEditDialog'
        with patch(target, new=ListEditMock):
            self.controller.set_read_only(False)
            self.controller._on_edit_clicked()
            assert all(self.proxy.edit_value == [-1, 42, -1])

    def test_size_list(self):
        assert get_min_max_size(self.size_proxy.binding) == (1, 3)
        self.size_controller.set_read_only(False)
        self.size_controller._internal_widget.setText('3,4')
        assert all(self.size_proxy.edit_value == [3, 4])
        self.size_controller._internal_widget.setText('3,4,5,6')
        assert self.size_proxy.edit_value is None
        self.size_controller._internal_widget.setText('')
        assert self.size_proxy.edit_value is None

    def test_trailing_zeros(self):
        assert get_min_max_size(self.size_proxy.binding) == (1, 3)
        self.size_controller.set_read_only(False)
        self.size_controller._internal_widget.setText('03,4')
        assert self.size_proxy.edit_value is None
        self.size_controller._internal_widget.setText('3,4')
        assert all(self.size_proxy.edit_value == [3, 4])
        self.size_controller._internal_widget.setText('000000,4')
        assert all(self.size_proxy.edit_value == [0, 4])

    def test_shorts(self):
        self.short_controller.set_read_only(False)
        self.short_controller._internal_widget.setText('03,4')
        assert self.short_proxy.edit_value is None
        self.short_controller._internal_widget.setText('3,4')
        assert all(self.short_proxy.edit_value == [3, 4])
        # out of range
        self.short_controller._internal_widget.setText('10300')
        assert self.short_proxy.edit_value is None


class TestEditableRegexList(GuiTestCase):
    def setUp(self):
        super(TestEditableRegexList, self).setUp()
        self.proxy = get_class_property_proxy(
            RegexObject.getClassSchema(), 'prop')
        self.controller = EditableRegexList(
            proxy=self.proxy, model=EditableRegexListModel())
        self.controller.create(None)
        self.controller.binding_update(self.proxy)
        apply_default_configuration(self.proxy.root_proxy.binding)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_binding_update_validator(self):
        assert self.controller._internal_widget.validator() is not None

    def test_set_value(self):
        self.controller.last_cursor_position = 0
        set_proxy_value(self.proxy, 'prop', ["remote:outputDaq"])
        assert self.controller._internal_widget.text() == "remote:outputDaq"
        assert self.controller._internal_widget.cursorPosition() == 0

    def test_edit_value(self):
        self.controller.set_read_only(False)
        self.controller._internal_widget.setText(
            "remote:outputDaq,remote:output")
        self.assertEqual(self.proxy.edit_value,
                         ["remote:outputDaq", "remote:output"])

    def test_edit_empty_value(self):
        self.controller.set_read_only(False)
        self.controller._internal_widget.setText('remote:output')
        self.assertEqual(self.proxy.edit_value, ["remote:output"])
        self.controller._internal_widget.setText('')
        value = self.proxy.edit_value
        self.assertEqual(value, [])
