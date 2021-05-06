from karabo.native import Bool, ChoiceOfNodes, Configurable
from karabogui.binding.api import apply_default_configuration
from karabogui.testing import GuiTestCase, get_class_property_proxy

from ..choiceelement import DisplayChoiceElement


class ChoicesBase(Configurable):
    pass


class ChoiceOne(ChoicesBase):
    prop = Bool()


class ChoiceTwo(ChoicesBase):
    prop = Bool()


class Object(Configurable):
    prop = ChoiceOfNodes(ChoicesBase, defaultValue='ChoiceOne')


class TestDisplayChoiceElement(GuiTestCase):
    def setUp(self):
        super(TestDisplayChoiceElement, self).setUp()

        schema = Object.getClassSchema()
        self.proxy = get_class_property_proxy(schema, 'prop')
        self.controller = DisplayChoiceElement(proxy=self.proxy)
        self.controller.create(None)

    def tearDown(self):
        super(TestDisplayChoiceElement, self).tearDown()
        self.controller.destroy()
        assert self.controller.widget is None

    def test_setup(self):
        assert self.controller.widget.count() == 2

    def test_set_value(self):
        apply_default_configuration(self.proxy.root_proxy.binding)
        assert self.controller.widget.currentText() == 'ChoiceOne'

        self.proxy.value = 'ChoiceTwo'
        assert self.controller.widget.currentText() == 'ChoiceTwo'
