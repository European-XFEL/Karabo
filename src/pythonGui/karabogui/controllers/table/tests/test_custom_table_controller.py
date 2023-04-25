# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from unittest import main

from qtpy.QtCore import QPoint

from karabo.common.scenemodel.api import TableElementModel
from karabo.native import Bool, Configurable, Hash, String, VectorHash
from karabogui.binding.config import apply_configuration
from karabogui.controllers.table.api import BaseTableController, TableModel
from karabogui.testing import GuiTestCase, get_property_proxy


class TableSchema(Configurable):
    arch = String(
        defaultValue="NoString")
    foo = Bool(
        defaultValue=False,
        displayedName="Foo")


class Object(Configurable):
    prop = VectorHash(rows=TableSchema)


class MyTableModel(TableModel):
    state_less = True


count = 0
filter_model = False
legacy_model = False


class MyTableController(BaseTableController):
    # A table model without resizeToContents
    hasCustomMenu = True
    tableModelClass = MyTableModel

    def custom_menu(self, pos):
        global count
        count += 1

    def createFilterModel(self, item_model):
        global filter_model
        filter_model = True
        return item_model


class LegacyTableController(BaseTableController):
    tableModelClass = MyTableModel

    def createModel(self, item_model):
        global legacy_model
        legacy_model = True
        return item_model


class TestCustomBaseController(GuiTestCase):
    def setUp(self):
        super().setUp()
        self.proxy = get_property_proxy(Object.getClassSchema(), "prop")
        self.model = TableElementModel()
        self.controller = MyTableController(proxy=self.proxy,
                                            model=self.model)
        self.controller.create(None)
        self.assertTrue(self.controller.isReadOnly())
        self.controller.set_read_only(False)
        self.table_hash = Hash(
            "prop",
            [Hash("arch", "a", "foo", True),
             Hash("arch", "b", "foo", False)])
        apply_configuration(self.table_hash, self.proxy.root_proxy.binding)

    def tearDown(self):
        super().tearDown()
        self.controller.destroy()

    def test_subclassing_model(self):
        """Test that one can specify the table model"""
        model = self.controller.sourceModel()
        self.assertTrue(model.state_less)
        self.assertIsInstance(model, MyTableModel)
        self.assertEqual(filter_model, True)

    def test_custom_context(self):
        """Test that one can use a custom context menu"""
        controller = self.controller
        # Note: Try QTest on later versions with rightclick, not working in
        # Qt 5.9
        self.assertEqual(count, 0)
        controller._custom_menu(QPoint(0, 0))
        # Calls custom_menu
        self.assertEqual(count, 1)

    def test_column_index_key(self):
        index = self.controller.columnIndex("foo")
        self.assertEqual(index, 1)
        index = self.controller.columnIndex("arch")
        self.assertEqual(index, 0)
        # Schema evolution! Return `None` index
        index = self.controller.columnIndex("nothere")
        self.assertIsNone(index)

        key = self.controller.columnKey(0)
        self.assertEqual(key, "arch")
        key = self.controller.columnKey(1)
        self.assertEqual(key, "foo")
        key = self.controller.columnKey(2)
        self.assertIsNone(key)

    def test_stretch_last_section(self):
        table_widget = self.controller.tableWidget()
        self.assertTrue(table_widget.horizontalHeader().stretchLastSection())

    def test_legacy_controller(self):
        proxy = get_property_proxy(Object.getClassSchema(), "prop")
        model = TableElementModel()
        controller = LegacyTableController(proxy=proxy,
                                           model=model)
        controller.create(None)
        self.assertTrue(controller.isReadOnly())
        self.assertTrue(legacy_model)

        # Check the menu
        menu = controller.get_basic_menu()
        self.assertIsNotNone(menu)
        # Readonly does not have an action
        self.assertEqual(len(menu.actions()), 0)
        controller.set_read_only(False)
        menu = controller.get_basic_menu()
        self.assertIsNotNone(menu)
        self.assertEqual(len(menu.actions()), 1)


if __name__ == "__main__":
    main()
