# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.common.scenemodel.api import TableElementModel
from karabo.native import Bool, Configurable, Hash, Int32, String, VectorHash
from karabogui.binding.config import apply_configuration
from karabogui.testing import GuiTestCase, get_class_property_proxy

from ..table import EditableTableElement


class Row(Configurable):
    foo = Bool(displayedName='Foo')
    bar = String(displayedName='Bar')
    cat = Int32(displayedName="Cat", options=[1, 2, 3])


class Object(Configurable):
    prop = VectorHash(rows=Row)


TABLE_HASH = Hash('prop', [Hash('foo', True, 'bar', 'hello', 'cat', 1),
                           Hash('foo', False, 'bar', 'test', 'cat', 3),
                           Hash('foo', False, 'bar', 'No', 'cat', 2),
                           Hash('foo', True, 'bar', 'Jo', 'cat', 1)])


class TestEditableTableElement(GuiTestCase):
    def setUp(self):
        super(TestEditableTableElement, self).setUp()
        self.proxy = get_class_property_proxy(Object.getClassSchema(), 'prop')
        self.model = TableElementModel()
        self.controller = EditableTableElement(proxy=self.proxy,
                                               model=self.model)
        self.controller.create(None)
        apply_configuration(TABLE_HASH, self.proxy.root_proxy.binding)

    def tearDown(self):
        self.controller.destroy()
        assert self.controller.widget is None

    def assertModelRow(self, row, first_column, second_column, third_column,
                       size=None):
        model = self.controller._item_model
        self.assertEqual(model.index(row, 0).data(), first_column)
        self.assertEqual(model.index(row, 1).data(), second_column)
        self.assertEqual(model.index(row, 2).data(), str(third_column))

        data = self.controller._item_model._data
        bool_cast = first_column == "True"
        self.assertEqual(data[row], Hash('foo', bool_cast,
                                         'bar', second_column,
                                         'cat', third_column))
        if size is not None:
            self.assertEqual(data, size)

    def test_set_value(self):
        self.assertModelRow(0, 'True', 'hello', 1)
        self.assertModelRow(1, 'False', 'test', 3)
        self.assertModelRow(2, 'False', 'No', 2)
        self.assertModelRow(3, 'True', 'Jo', 1)

    def test_empty_value(self):
        model = self.controller._item_model
        self.assertNotEqual(model._data, [])
        model.clear_model()
        self.assertEqual(model._data, [])

    def test_edit_value(self):
        self.controller.set_read_only(False)
