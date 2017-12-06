from karabo.common.scenemodel.api import TableElementModel
from karabo.middlelayer import Configurable, Bool, Hash, String, VectorHash
from karabogui.testing import GuiTestCase, get_class_property_proxy
from ..table import EditableTableElement


class Row(Configurable):
    foo = Bool()
    bar = String()


class Object(Configurable):
    prop = VectorHash(rows=Row)


class TestEditableTableElement(GuiTestCase):
    def setUp(self):
        super(TestEditableTableElement, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.model = TableElementModel()
        self.controller = EditableTableElement(proxy=self.proxy,
                                               model=self.model)
        self.controller.create(None)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def test_set_value(self):
        self.proxy.value = [Hash('foo', True, 'bar', 'hello'),
                            Hash('foo', False, 'bar', 'test')]

    def test_edit_value(self):
        self.controller.set_read_only(False)
