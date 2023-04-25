# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from qtpy.QtCore import Qt

from karabo.common.scenemodel.api import CheckBoxModel
from karabo.common.states import State
from karabo.native import Bool, Configurable, String
from karabogui.testing import (
    GuiTestCase, get_class_property_proxy, set_proxy_value)

from ..checkbox import EditableCheckBox


class Object(Configurable):
    state = String(defaultValue=State.ON)
    prop = Bool(allowedStates=[State.ON])


class TestEditableCheckBox(GuiTestCase):
    def setUp(self):
        super(TestEditableCheckBox, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.controller = EditableCheckBox(proxy=self.proxy,
                                           model=CheckBoxModel())
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_state_update(self):
        set_proxy_value(self.proxy, 'state', 'CHANGING')
        assert self.controller.widget.isEnabled() is False
        set_proxy_value(self.proxy, 'state', 'ON')
        assert self.controller.widget.isEnabled() is True

    def test_set_value(self):
        set_proxy_value(self.proxy, 'prop', True)
        assert self.controller.widget.checkState() == Qt.Checked

    def test_edit_value(self):
        self.controller.widget.setCheckState(Qt.Checked)
        assert self.proxy.edit_value

        self.controller.widget.setCheckState(Qt.Unchecked)
        assert not self.proxy.edit_value
