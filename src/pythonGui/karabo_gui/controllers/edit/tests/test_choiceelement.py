from karabo.middlelayer import Configurable, Bool, ChoiceOfNodes
from karabo_gui.binding.api import apply_default_configuration
from karabo_gui.testing import GuiTestCase, get_class_property_proxy
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

    def test_setup(self):
        assert self.controller.widget.count() == 2

    def test_set_value(self):
        apply_default_configuration(self.proxy.root_proxy.binding)
        assert self.controller.widget.currentText() == 'ChoiceOne'

        self.proxy.binding.choice = 'ChoiceTwo'
        assert self.controller.widget.currentText() == 'ChoiceTwo'

    def test_edit_value(self):
        widget = self.controller.widget
        index = widget.findText('ChoiceTwo')

        widget.setCurrentIndex(index)
        assert self.proxy.binding.choice == 'ChoiceTwo'
