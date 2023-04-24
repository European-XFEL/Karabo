# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from qtpy.QtCore import Qt

from karabo.native import Bool, ChoiceOfNodes, Configurable
from karabogui.binding.api import apply_default_configuration
from karabogui.testing import GuiTestCase, get_class_property_proxy

from ..choiceelement import EditableChoiceElement


class ChoicesBase(Configurable):
    pass


class ChoiceOne(ChoicesBase):
    prop = Bool()


class ChoiceTwo(ChoicesBase):
    prop = Bool()


class Object(Configurable):
    prop = ChoiceOfNodes(ChoicesBase, defaultValue='ChoiceOne')


class TestEditableChoiceElement(GuiTestCase):
    def setUp(self):
        super(TestEditableChoiceElement, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = EditableChoiceElement(proxy=self.proxy)
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_focus_policy(self):
        assert self.controller.widget.focusPolicy() == Qt.StrongFocus

    def test_setup(self):
        assert self.controller.widget.count() == 2

    def test_set_value(self):
        apply_default_configuration(self.proxy.root_proxy.binding)
        assert self.controller.widget.currentText() == 'ChoiceOne'

        self.proxy.value = 'ChoiceTwo'
        assert self.controller.widget.currentText() == 'ChoiceTwo'

    def test_edit_value(self):
        widget = self.controller.widget
        index = widget.findText('ChoiceTwo')

        widget.setCurrentIndex(index)
        assert self.proxy.binding.choice == 'ChoiceTwo'
