# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from qtpy.QtCore import Qt

from karabo.common.scenemodel.api import EditableComboBoxModel
from karabo.common.states import State
from karabo.native import Configurable, Int32, String
from karabogui.binding.api import build_binding
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..combobox import EditableComboBox


class Object(Configurable):
    state = String(defaultValue=State.ON)
    prop = String(options=['foo', 'bar', 'baz', 'qux'],
                  allowedStates=[State.ON])


class Other(Configurable):
    prop = Int32(options=[1, 2, 3, 5, 8])


class TestEditableComboBox(GuiTestCase):
    def setUp(self):
        super(TestEditableComboBox, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.controller = EditableComboBox(proxy=self.proxy,
                                           model=EditableComboBoxModel())
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_allowed(self):
        set_proxy_value(self.proxy, 'state', 'CHANGING')
        assert self.controller.widget.isEnabled() is False
        set_proxy_value(self.proxy, 'state', 'ON')
        assert self.controller.widget.isEnabled() is True

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', 'bar')
        assert self.controller.widget.currentIndex() == 1

        set_proxy_value(self.proxy, 'prop', 'nobar')
        assert self.controller.widget.currentIndex() == -1

        set_proxy_value(self.proxy, 'prop', 'baz')
        assert self.controller.widget.currentIndex() == 2

    def test_edit_value(self):
        self.controller.widget.setCurrentIndex(3)
        assert self.proxy.edit_value == 'qux'

    def test_schema_update(self):
        proxy = get_class_property_proxy(Other.getClassSchema(), 'prop')
        controller = EditableComboBox(proxy=proxy)
        controller.create(None)
        assert controller.widget.currentIndex() == -1
        set_proxy_value(proxy, 'prop', 2)
        assert controller.widget.currentIndex() == 1

        assert controller.widget.count() == 5

        build_binding(Object.getClassSchema(),
                      existing=proxy.root_proxy.binding)
        assert controller.widget.currentIndex() == -1
        assert controller.widget.count() == 4

    def test_focus(self):
        combobox = self.controller.widget
        assert combobox.focusPolicy() == Qt.StrongFocus
