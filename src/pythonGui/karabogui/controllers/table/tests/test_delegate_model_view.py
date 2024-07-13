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

from qtpy.QtWidgets import QStyledItemDelegate

from karabo.common.scenemodel.api import TableElementModel
from karabo.native import (
    AccessMode, Bool, Configurable, Double, Hash, Int32, String, VectorDouble,
    VectorHash)
from karabogui.binding.config import apply_configuration
from karabogui.controllers.table.api import (
    BaseTableController, BoolButtonDelegate, ColorBindingDelegate,
    ColorNumberDelegate, ProgressBarDelegate, VectorButtonDelegate)
from karabogui.testing import get_property_proxy


class TableSchema(Configurable):
    boolButton = Bool(
        defaultValue=False,
        displayType="TableBoolButton",
        displayedName="Bool Button",
        accessMode=AccessMode.READONLY)
    colorText = String(
        displayType="TableColor|default=red&ginger=blue&bailey=green",
        displayedName="Color Text",
        defaultValue="Indiana",
        accessMode=AccessMode.READONLY)
    colorNumber = Int32(
        defaultValue=1,
        displayType="TableColor|1=blue&2=red&default=black")
    progress = Double(
        displayedName="Progress",
        displayType="TableProgressBar",
        defaultValue=0.0,
        minInc=0.0,
        maxInc=100.0)
    vector = VectorDouble(
        displayedName="VectorDouble",
        displayType="TableVector",
        defaultValue=[])


class Object(Configurable):
    prop = VectorHash(rows=TableSchema)


class KaraboDelegate(QStyledItemDelegate):
    """A delegate class for testing"""


class TableDelegateController(BaseTableController):
    def create_delegates(self):
        delegate = KaraboDelegate(parent=self.tableWidget())
        self.setTableDelegates({0: delegate})


def test_table_delegate_model_view(gui_app):
    """Test the table controller with delegates"""
    proxy = get_property_proxy(Object.getClassSchema(), "prop")
    model = TableElementModel()
    controller = BaseTableController(proxy=proxy,
                                     model=model)
    controller.create(None)
    assert controller.isReadOnly()
    controller.set_read_only(False)
    table_hash = Hash(
        "prop",
        [Hash("boolButton", True, "colorText", "ginger",
              "colorNumber", 10, "progress", 75.0,
              "vector", [1.2, 1.3])])
    apply_configuration(table_hash, proxy.root_proxy.binding)

    widget = controller.tableWidget()
    delegate = widget.itemDelegateForColumn(0)
    assert not isinstance(delegate, BoolButtonDelegate)
    delegate = widget.itemDelegateForColumn(1)
    assert isinstance(delegate, ColorBindingDelegate)
    delegate = widget.itemDelegateForColumn(2)
    assert isinstance(delegate, ColorNumberDelegate)
    delegate = widget.itemDelegateForColumn(3)
    assert isinstance(delegate, ProgressBarDelegate)
    delegate = widget.itemDelegateForColumn(4)
    assert isinstance(delegate, VectorButtonDelegate)

    controller.set_read_only(True)
    delegate = widget.itemDelegateForColumn(0)
    assert isinstance(delegate, BoolButtonDelegate)
    delegate = widget.itemDelegateForColumn(4)
    assert not isinstance(delegate, VectorButtonDelegate)


def test_custom_delegates_controller(gui_app):
    proxy = get_property_proxy(Object.getClassSchema(), "prop")
    model = TableElementModel()
    controller = TableDelegateController(proxy=proxy,
                                         model=model)
    controller.create(None)
    controller.set_read_only(False)
    widget = controller.tableWidget()
    # First column changed
    delegate = widget.itemDelegateForColumn(0)
    assert isinstance(delegate, KaraboDelegate)
    # Others remain have been created as expected
    delegate = widget.itemDelegateForColumn(1)
    assert isinstance(delegate, ColorBindingDelegate)
    delegate = widget.itemDelegateForColumn(2)
    assert isinstance(delegate, ColorNumberDelegate)
    delegate = widget.itemDelegateForColumn(3)
    assert isinstance(delegate, ProgressBarDelegate)
