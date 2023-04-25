# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest import mock, skipIf

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QDialog

from karabo.common.scenemodel.api import EditableSpinBoxModel
from karabo.native import Configurable, Int32, UInt8, Unit
from karabogui.binding.api import build_binding
from karabogui.const import IS_MAC_SYSTEM
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..intspinbox import EditableSpinBox


class Object(Configurable):
    prop = Int32(minInc=-10, maxInc=10, unitSymbol=Unit.SECOND)


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
        assert self.controller.widget.value() == 5

    def test_edit_value(self):
        self.controller.widget.setValue(3)
        assert self.proxy.edit_value == 3

    def test_focus_policy(self):
        assert self.controller.widget.focusPolicy() == Qt.StrongFocus

    def test_schema_update(self):
        proxy = get_class_property_proxy(Other.getClassSchema(), 'prop')
        controller = EditableSpinBox(proxy=proxy)
        controller.create(None)

        assert controller.widget.minimum() == 0
        assert controller.widget.maximum() == 255
        assert controller.widget.suffix() == ""

        build_binding(Object.getClassSchema(),
                      existing=proxy.root_proxy.binding)

        assert controller.widget.minimum() == -10
        assert controller.widget.maximum() == 10
        assert controller.widget.suffix() == " s"

    @skipIf(IS_MAC_SYSTEM, "Fonts are different on MacOS")
    def test_font_setting(self):
        settings = "{ font: normal; font-size: 10pt; }"
        assert settings in self.controller.widget.styleSheet()

        path = "karabogui.controllers.edit.intspinbox.FormatLabelDialog"
        with mock.patch(path) as dialog:
            dialog().exec.return_value = QDialog.Accepted
            dialog().font_size = 11
            dialog().font_weight = "bold"
            self.controller._format_field()

            settings = "{ font: bold; font-size: 11pt; }"
            assert settings in self.controller.widget.styleSheet()
