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
from karabo.native import Configurable, Hash, String, VectorHash
from karabogui.configurator.api import TableDialog
from karabogui.testing import (
    click_button, get_class_property_proxy, set_proxy_hash)


class TableSchema(Configurable):
    foo = String(
        defaultValue="NoString")
    bar = String(
        displayType="State",
        displayedName="Bar",
        defaultValue="ON")


class BigTableSchema(Configurable):
    a = String()
    b = String()
    c = String()
    d = String()
    e = String()


TABLE_HASH = Hash("prop", [Hash("foo", "1", "bar", "hello"),
                           Hash("foo", "2", "bar", "test"),
                           Hash("foo", "3", "bar", "No"),
                           Hash("foo", "4", "bar", "Jo")])


class Object(Configurable):
    prop = VectorHash(rows=TableSchema)
    bigProp = VectorHash(rows=BigTableSchema)


def test_table_view_dialog(gui_app, subtests):
    proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
    dialog = TableDialog(proxy, True)
    assert dialog is not None
    assert dialog.width() == 450
    set_proxy_hash(proxy, TABLE_HASH)

    assert dialog.width() == 450
    # Finish dialog, destroy widget
    dialog.done(1)
    assert dialog.controller is not None
    assert dialog.controller.widget is None

    # New dialog, with big table schema
    proxy = get_class_property_proxy(Object.getClassSchema(), "bigProp")
    dialog = TableDialog(proxy, True)
    assert dialog is not None
    assert dialog.width() == 600
    # Finish dialog without success
    dialog.done(0)
    assert dialog.controller is not None
    assert dialog.controller.widget is None
    assert dialog.toolbar is None

    # New dialog, launch actions
    with subtests.test("Test the table view action buttons"):
        proxy = get_class_property_proxy(Object.getClassSchema(), "prop")
        dialog = TableDialog(proxy, True)
        assert dialog is not None
        assert dialog.width() == 450
        set_proxy_hash(proxy, TABLE_HASH)
        assert dialog.controller.sourceModel() is not None

        data = dialog.controller.sourceModel().index(0, 0).data()
        assert data == "1"
        dialog.controller.tableWidget().selectRow(0)
        click_button(dialog.toolbar._move_down_button)
        data = dialog.controller.sourceModel().index(0, 0).data()
        assert data == "2"
        dialog.controller.tableWidget().selectRow(1)
        click_button(dialog.toolbar._move_up_button)
        data = dialog.controller.sourceModel().index(0, 0).data()
        assert data == "1"

        click_button(dialog.toolbar._remove_button)
        data = dialog.controller.sourceModel().index(0, 0).data()
        assert data == "2"

        assert dialog.controller.sourceModel().rowCount() == 3
        dialog.controller.tableWidget().selectRow(2)
        data = dialog.controller.sourceModel().index(2, 0).data()
        assert data == "4"
        # Adding adds a default value
        click_button(dialog.toolbar._add_button)
        assert dialog.controller.sourceModel().rowCount() == 4
        data = dialog.controller.sourceModel().index(3, 0).data()
        assert data == "NoString"

        # Duplicate copies
        dialog.controller.tableWidget().selectRow(2)
        click_button(dialog.toolbar._du_button)
        assert dialog.controller.sourceModel().rowCount() == 5
        data = dialog.controller.sourceModel().index(3, 0).data()
        assert data == "4"
