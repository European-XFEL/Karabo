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

from karabo.common.scenemodel.api import TableElementModel
from karabo.native import Bool, Configurable, Hash, Int32, String, VectorHash
from karabogui.binding.config import apply_configuration
from karabogui.testing import get_class_property_proxy

from ..table import EditableTableElement


class Row(Configurable):
    foo = Bool(displayedName="Foo")
    bar = String(displayedName="Bar")
    cat = Int32(displayedName="Cat", options=[1, 2, 3])


class Object(Configurable):
    prop = VectorHash(rows=Row)


TABLE_HASH = Hash("prop", [Hash("foo", True, "bar", "hello", "cat", 1),
                           Hash("foo", False, "bar", "test", "cat", 3),
                           Hash("foo", False, "bar", "No", "cat", 2),
                           Hash("foo", True, "bar", "Jo", "cat", 1)])


@pytest.fixture
def table_setup(gui_app):
    proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
    model = TableElementModel()
    controller = EditableTableElement(proxy=proxy, model=model)
    controller.create(None)
    apply_configuration(TABLE_HASH, proxy.root_proxy.binding)
    yield controller
    # teardown
    controller.destroy()
    assert controller.widget is None


def assert_model_row(controller, row, first_column, second_column,
                     third_column, size=None):
    model = controller._item_model
    assert model.index(row, 0).data() == first_column
    assert model.index(row, 1).data() == second_column
    assert model.index(row, 2).data() == str(third_column)

    data = controller._item_model._data
    bool_cast = first_column == "True"
    assert data[row] == Hash("foo", bool_cast,
                             "bar", second_column,
                             "cat", third_column)
    if size is not None:
        assert data == size


def test_table_set_value(table_setup):
    controller = table_setup
    assert_model_row(controller, 0, "True", "hello", 1)
    assert_model_row(controller, 1, "False", "test", 3)
    assert_model_row(controller, 2, "False", "No", 2)
    assert_model_row(controller, 3, "True", "Jo", 1)


def test_table_empty_value(table_setup):
    controller = table_setup
    model = controller._item_model
    assert model._data != []
    model.clear_model()
    assert model._data == []


def test_table_edit_value(table_setup):
    controller = table_setup
    controller.set_read_only(False)
