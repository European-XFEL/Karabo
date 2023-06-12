# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import pytest
from qtpy.QtCore import QPoint

from karabo.common.scenemodel.api import TableElementModel
from karabo.native import Bool, Configurable, Hash, String, VectorHash
from karabogui.binding.config import apply_configuration
from karabogui.controllers.table.api import BaseTableController, TableModel
from karabogui.testing import get_property_proxy


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


@pytest.fixture
def custom_table_controller_setup(gui_app):
    proxy = get_property_proxy(Object.getClassSchema(), "prop")
    model = TableElementModel()
    controller = MyTableController(proxy=proxy, model=model)
    controller.create(None)
    assert controller.isReadOnly()
    controller.set_read_only(False)
    table_hash = Hash(
        "prop",
        [Hash("arch", "a", "foo", True),
         Hash("arch", "b", "foo", False)])
    apply_configuration(table_hash, proxy.root_proxy.binding)

    yield controller

    controller.destroy()
    assert controller.widget is None


def test_subclassing_model(custom_table_controller_setup):
    """Test that one can specify the table model"""
    controller = custom_table_controller_setup
    model = controller.sourceModel()
    assert model.state_less
    assert isinstance(model, MyTableModel)
    assert filter_model is True


def test_custom_context(custom_table_controller_setup):
    """Test that one can use a custom context menu"""
    controller = custom_table_controller_setup
    # Note: Try QTest on later versions with rightclick, not working in
    # Qt 5.9
    assert count == 0
    controller._custom_menu(QPoint(0, 0))
    # Calls custom_menu
    assert count == 1


def test_column_index_key(custom_table_controller_setup):
    controller = custom_table_controller_setup
    index = controller.columnIndex("foo")
    assert index == 1
    index = controller.columnIndex("arch")
    assert index == 0
    # Schema evolution! Return `None` index
    index = controller.columnIndex("nothere")
    assert index is None

    key = controller.columnKey(0)
    assert key == "arch"
    key = controller.columnKey(1)
    assert key == "foo"
    key = controller.columnKey(2)
    assert key is None


def test_stretch_last_section(custom_table_controller_setup):
    controller = custom_table_controller_setup
    table_widget = controller.tableWidget()
    assert table_widget.horizontalHeader().stretchLastSection()


def test_legacy_controller(gui_app):
    proxy = get_property_proxy(Object.getClassSchema(), "prop")
    model = TableElementModel()
    controller = LegacyTableController(proxy=proxy, model=model)
    controller.create(None)
    assert controller.isReadOnly()
    assert legacy_model

    # Check the menu
    menu = controller.get_basic_menu()
    assert menu is not None
    # Readonly does not have an action
    assert len(menu.actions()) == 0
    controller.set_read_only(False)
    menu = controller.get_basic_menu()
    assert menu is not None
    assert len(menu.actions()) == 1
