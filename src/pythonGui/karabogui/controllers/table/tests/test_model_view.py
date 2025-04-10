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
from qtpy.QtCore import (
    QEvent, QItemSelectionModel, QMimeData, QModelIndex, QPoint, Qt)
from qtpy.QtGui import QDropEvent, QKeySequence

from karabo.common.api import State
from karabo.common.scenemodel.api import (
    FilterTableElementModel, TableElementModel)
from karabo.native import (
    AccessLevel, AccessMode, Bool, Configurable, Double, Hash, Int32,
    MetricPrefix, String, Unit, VectorHash, VectorString)
from karabogui.binding.config import apply_configuration
from karabogui.controllers.table.api import (
    BaseFilterTableController, BaseTableController, KaraboTableView,
    StringButtonDelegate, TableButtonDelegate, create_mime_data, list2string)
from karabogui.testing import (
    access_level, get_property_proxy, keySequence, set_proxy_value, singletons)
from karabogui.topology.api import SystemTopology

DELEGATES = "karabogui.controllers.table.delegates"


class ButtonSchema(Configurable):
    arch = String(
        displayType="TableStringButton",
        defaultValue="deviceScene|device_id=XHQ_EG_DG/CAM/CAMERA&name=scene")
    bar = String(
        displayType="TableStringButton",
        defaultValue="url|http://www.xfel.eu")
    foo = Bool(
        displayType="TableBoolButton",
        defaultValue=False,
        displayedName="Foo")


class TableSchema(Configurable):
    arch = String(
        defaultValue="NoString")
    foo = Bool(
        defaultValue=False,
        displayedName="Foo")
    bar = String(
        enum=State,
        displayType="State",
        displayedName="Bar",
        defaultValue="ON",
        accessMode=AccessMode.READONLY)
    cat = Int32(
        defaultValue=1,
        displayedName="Cat",
        options=[1, 2, 3])
    dog = Double(
        displayedName="Dog",
        defaultValue=2.0)
    eagle = VectorString(
        displayedName="Eagle",
        description="This is a description for a table",
        unitSymbol=Unit.METER,
        metricPrefixSymbol=MetricPrefix.MILLI,
        defaultValue=[])


class Object(Configurable):
    prop = VectorHash(
        rows=TableSchema,
        accessMode=AccessMode.READONLY)


class ButtonObject(Configurable):
    prop = VectorHash(
        rows=ButtonSchema)


class StateObject(Configurable):
    state = String(
        defaultValue=State.ON)
    prop = VectorHash(
        rows=ButtonSchema,
        requiredAccessLevel=AccessLevel.OPERATOR,
        accessMode=AccessMode.READONLY,
        allowedStates=[State.ON])


def assert_model_row(controller, row, first_column, second_column,
                     third_column, fourth_column, fifth_column, sixth_column,
                     size=None):
    model = controller.sourceModel()
    # Boolean is represented as Qt value in table model

    assert model.index(row, 0).data() == first_column
    qt_bool = Qt.Checked if second_column else Qt.Unchecked
    assert model.index(row, 1).data(role=Qt.CheckStateRole) == qt_bool
    assert model.index(row, 2).data() == third_column
    # integer and floating point are string data to outside
    assert model.index(row, 3).data() == str(fourth_column)
    assert model.index(row, 4).data() == str(fifth_column)
    # Vector data is always retrieved as string list
    assert model.index(row, 5).data() == list2string(sixth_column)

    data = controller.sourceModel()._data
    # Our Hash still has real booleans, but the model does not!
    assert data[row], Hash("arch", first_column,
                           "foo", second_column,
                           "bar", third_column,
                           "cat", fourth_column,
                           "dog", fifth_column,
                           "eagle", sixth_column)
    if size is not None:
        assert data == size


@pytest.fixture
def table_model_view_setup(gui_app):
    proxy = get_property_proxy(Object.getClassSchema(), "prop")
    model = TableElementModel()
    controller = BaseTableController(proxy=proxy, model=model)
    controller.create(None)
    assert controller.isReadOnly()
    controller.set_read_only(False)
    table_hash = Hash(
        "prop",
        [Hash("arch", "a", "foo", True, "bar", "ACTIVE", "cat", 1,
              "dog", 2.0, "eagle", ["a"]),
         Hash("arch", "b", "foo", False, "bar", "ERROR", "cat", 3,
              "dog", 4.0, "eagle", ["b"]),
         Hash("arch", "c", "foo", False, "bar", "UNKNOWN", "cat", 2,
              "dog", 1.0, "eagle", ["c"]),
         Hash("arch", "d", "foo", True, "bar", "CHANGING", "cat", 1,
              "dog", 5.0, "eagle", ["d"])])
    apply_configuration(table_hash, proxy.root_proxy.binding)
    yield controller, proxy, table_hash
    # teardown
    controller.destroy()


def test_table_model_view_focus_policy(table_model_view_setup):
    controller, _, _ = table_model_view_setup
    assert controller.widget.focusPolicy() == Qt.StrongFocus


def test_table_model_view_deviceId(table_model_view_setup):
    controller, _, _ = table_model_view_setup
    deviceId = controller.getInstanceId()
    assert deviceId == "TestDevice"


def test_table_model_view_set_value(table_model_view_setup):
    controller, _, _ = table_model_view_setup
    assert_model_row(controller, 0, "a", True, "ACTIVE", 1, 2.0, ["a"])
    assert_model_row(controller, 1, "b", False, "ERROR", 3, 4.0, ["b"])
    assert_model_row(controller, 2, "c", False, "UNKNOWN", 2, 1.0, ["c"])
    assert_model_row(controller, 3, "d", True, "CHANGING", 1, 5.0, ["d"])


def test_table_model_view_header(table_model_view_setup):
    controller, _, _ = table_model_view_setup
    model = controller.sourceModel()
    header_keys = model._header
    assert header_keys == ["arch", "foo", "bar", "cat", "dog", "eagle"]
    foo = model.headerData(1, orientation=Qt.Horizontal,
                           role=Qt.DisplayRole)
    assert foo == "Foo"
    bar = model.headerData(2, orientation=Qt.Horizontal,
                           role=Qt.DisplayRole)
    assert bar == "Bar"
    cat = model.headerData(3, orientation=Qt.Horizontal,
                           role=Qt.DisplayRole)
    assert cat == "Cat"
    dog = model.headerData(4, orientation=Qt.Horizontal,
                           role=Qt.DisplayRole)
    assert dog == "Dog"
    eagle = model.headerData(5, orientation=Qt.Horizontal,
                             role=Qt.DisplayRole)
    assert eagle == "Eagle [mm]"
    is_none = model.headerData(1, orientation=Qt.Horizontal,
                               role=Qt.EditRole)
    assert is_none is None
    is_none = model.headerData(1, orientation=Qt.Vertical,
                               role=Qt.EditRole)
    assert is_none is None

    v = model.headerData(0, orientation=Qt.Vertical,
                         role=Qt.DisplayRole)
    assert v == "0"


def test_table_model_view_move_row_up(table_model_view_setup):
    controller, _, _ = table_model_view_setup
    assert_model_row(controller, 0, "a", True, "ACTIVE", 1, 2.0, ["a"])
    assert_model_row(controller, 1, "b", False, "ERROR", 3, 4.0, ["b"])

    # Move it
    model = controller.sourceModel()
    model.move_row_up(1)

    assert_model_row(controller, 0, "b", False, "ERROR", 3, 4.0, ["b"])
    assert_model_row(controller, 1, "a", True, "ACTIVE", 1, 2.0, ["a"])

    # --- Corner case ---
    model.move_row_up(0)
    assert_model_row(controller, 0, "b", False, "ERROR", 3, 4.0, ["b"])
    assert_model_row(controller, 1, "a", True, "ACTIVE", 1, 2.0, ["a"])


def test_table_model_view_move_row_down(table_model_view_setup):
    controller, _, _ = table_model_view_setup

    assert_model_row(controller, 0, "a", True, "ACTIVE", 1, 2.0, ["a"])
    assert_model_row(controller, 1, "b", False, "ERROR", 3, 4.0, ["b"])

    # Move it
    model = controller.sourceModel()
    model.move_row_down(0)

    assert_model_row(controller, 0, "b", False, "ERROR", 3, 4.0, ["b"])
    assert_model_row(controller, 1, "a", True, "ACTIVE", 1, 2.0, ["a"])
    assert_model_row(controller, 2, "c", False, "UNKNOWN", 2, 1.0, ["c"])
    assert_model_row(controller, 3, "d", True, "CHANGING", 1, 5.0, ["d"])

    # --- Corner case ---
    model.move_row_down(3)
    assert_model_row(controller, 0, "b", False, "ERROR", 3, 4.0, ["b"])
    assert_model_row(controller, 1, "a", True, "ACTIVE", 1, 2.0, ["a"])
    assert_model_row(controller, 2, "c", False, "UNKNOWN", 2, 1.0, ["c"])
    assert_model_row(controller, 3, "d", True, "CHANGING", 1, 5.0, ["d"])


def test_table_model_view_duplicate_row(table_model_view_setup):
    controller, _, _ = table_model_view_setup

    assert_model_row(controller, 0, "a", True, "ACTIVE", 1, 2.0, ["a"])
    assert_model_row(controller, 1, "b", False, "ERROR", 3, 4.0, ["b"])
    model = controller.sourceModel()
    model.duplicate_row(0)
    assert_model_row(controller, 0, "a", True, "ACTIVE", 1, 2.0, ["a"])
    assert_model_row(controller, 1, "a", True, "ACTIVE", 1, 2.0, ["a"])
    assert_model_row(controller, 2, "b", False, "ERROR", 3, 4.0, ["b"])


def test_table_model_view_empty_value(table_model_view_setup):
    controller, proxy, table_hash = table_model_view_setup
    model = controller.sourceModel()
    assert model._data != []
    model.clear_model()
    assert model._data == []
    assert model.rowCount(QModelIndex()) == 0
    # And a new round of applying configuration
    apply_configuration(table_hash, proxy.root_proxy.binding)
    assert model.rowCount(QModelIndex()) == 4
    # Set empty list
    apply_configuration(Hash("prop", []), proxy.root_proxy.binding)
    assert model.rowCount(QModelIndex()) == 0
    # Fill up again and decrease to 3
    apply_configuration(table_hash, proxy.root_proxy.binding)
    assert model.rowCount(QModelIndex()) == 4

    h = Hash(
        "prop",
        [Hash("arch", "a", "foo", True, "bar", "ACTIVE", "cat", 1,
              "dog", 2.0, "eagle", ["a"]),
         Hash("arch", "b", "foo", False, "bar", "ERROR", "cat", 3,
              "dog", 4.0, "eagle", ["b"]),
         Hash("arch", "c", "foo", False, "bar", "UNKNOWN", "cat", 2,
              "dog", 1.0, "eagle", ["c"])])
    apply_configuration(h, proxy.root_proxy.binding)
    assert model.rowCount(QModelIndex()) == 3

    # Test model data
    value = controller.get_model_data(0, 0)
    assert value == ("arch", "a")
    value = controller.get_model_data(0, 1)
    assert value == ("foo", True)
    value = controller.get_model_data(0, 2)
    assert value == ("bar", "ACTIVE")

    value = controller.get_model_data(1, 0)
    assert value == ("arch", "b")
    value = controller.getModelData(1, 0)
    assert value == ("arch", "b")


def test_table_model_view_background_value(table_model_view_setup):
    controller, _, _ = table_model_view_setup

    model = controller.sourceModel()
    index = model.index(0, 1)
    assert index.data(role=Qt.BackgroundRole) is None
    index = model.index(0, 2)
    brush = index.data(role=Qt.BackgroundRole)
    assert brush is not None
    color = brush.color().getRgb()
    assert color == (120, 255, 0, 255)
    # Set new state
    model.setData(index, value="ERROR", role=Qt.EditRole)
    brush = index.data(role=Qt.BackgroundRole)
    color = brush.color().getRgb()
    assert color == (255, 0, 0, 255)

    # Set wrong state
    model.setData(index, value=" NOSTATE", role=Qt.EditRole)
    brush = index.data(role=Qt.BackgroundRole)
    assert brush is None


def test_table_model_view_set_boolean_data(table_model_view_setup):
    controller, _, _ = table_model_view_setup
    model = controller.sourceModel()
    index = model.index(0, 1)
    assert_model_row(controller, 0, "a", True, "ACTIVE", 1, 2.0, ["a"])
    model.setData(index, value=Qt.Unchecked, role=Qt.CheckStateRole)
    assert_model_row(controller, 0, "a", False, "ACTIVE", 1, 2.0, ["a"])
    model.setData(index, value=Qt.Checked, role=Qt.CheckStateRole)
    assert_model_row(controller, 0, "a", True, "ACTIVE", 1, 2.0, ["a"])


def test_table_model_view_set_vector_data(table_model_view_setup):
    controller, _, _ = table_model_view_setup
    model = controller.sourceModel()
    assert_model_row(controller, 0, "a", True, "ACTIVE", 1, 2.0, ["a"])
    index = model.index(0, 5)
    # The model expects string data for lists
    model.setData(index, value=list2string(["a", "b", "c"]))
    data = index.data(role=Qt.DisplayRole)
    assert data == "a,b,c"
    with pytest.raises(AttributeError):
        # List does not have split attribute ...
        model.setData(index, value=["a", "b", "c"])


def test_table_model_view_tooltip_data(table_model_view_setup):
    controller, _, _ = table_model_view_setup
    model = controller.sourceModel()
    index = model.index(0, 5)
    data = index.data(role=Qt.ToolTipRole)
    assert data == "a"
    ret = model.setData(index, "Not allowed", role=Qt.ToolTipRole)
    assert not ret

    # Check the header
    data = model.headerData(section=5, orientation=Qt.Horizontal,
                            role=Qt.ToolTipRole)
    assert "key" in data
    assert "metricPrefixSymbol" in data
    assert "unitSymbol" in data
    assert "defaultValue" in data
    assert "valueType" in data
    assert "VECTOR_STRING" in data
    assert "description" in data
    assert "This is a description for a table" in data
    assert "minExc" not in data
    assert "minInc" not in data


def test_table_model_view_invalid_model_index(table_model_view_setup):
    controller, _, _ = table_model_view_setup
    model = controller.sourceModel()
    ret = model.setData(QModelIndex(), 2, Qt.EditRole)
    assert not ret
    assert model.data(QModelIndex()) is None
    assert model.flags(QModelIndex()) == 0


def test_table_model_view_model_flags(table_model_view_setup):
    controller, _, _ = table_model_view_setup
    model = controller.sourceModel()
    # 1. User checkable boolean
    index = model.index(0, 1)
    flags = index.flags()
    assert (flags & Qt.ItemIsUserCheckable) == Qt.ItemIsUserCheckable
    assert (flags & Qt.ItemIsEditable) != Qt.ItemIsEditable

    # 2. Readonly string
    index = model.index(0, 2)
    flags = index.flags()
    assert (flags & Qt.ItemIsUserCheckable) != Qt.ItemIsUserCheckable
    assert (flags & Qt.ItemIsEditable) != Qt.ItemIsEditable

    # 3. reconfigurable int
    index = model.index(0, 3)
    flags = index.flags()
    assert (flags & Qt.ItemIsUserCheckable) != Qt.ItemIsUserCheckable
    assert (flags & Qt.ItemIsEditable) == Qt.ItemIsEditable


def test_table_model_view_read_only_table(table_model_view_setup):
    controller, _, _ = table_model_view_setup
    controller.set_read_only(True)


def test_table_model_view_view_menu_select(table_model_view_setup, mocker):
    controller, _, _ = table_model_view_setup
    menu_path = "karabogui.controllers.table.controller.QMenu"
    pos = QPoint(0, 0)
    menu = mocker.patch(menu_path)
    model = controller.sourceModel()
    index = model.index(2, 0)

    view = controller.widget
    selection = view.selectionModel()
    selection.setCurrentIndex(index,
                              QItemSelectionModel.ClearAndSelect)
    # Assert the selection
    assert view.currentIndex() == index
    assert controller.currentIndex() == index

    controller._context_menu(pos)
    menu.assert_called_once()

    model.setData(index, "Yes A String")
    assert index.data() == "Yes A String"
    controller._set_index_default()
    assert index.data() == "NoString"

    # Invalid index also provides a model!
    menu.reset_mock()
    selection.setCurrentIndex(QModelIndex(),
                              QItemSelectionModel.ClearAndSelect)
    assert view.currentIndex() == QModelIndex()

    controller._context_menu(pos)
    menu.assert_called_once()


def test_table_model_view_drag_drop_view(table_model_view_setup):
    """Test the drag & drop of the table view"""
    controller, _, _ = table_model_view_setup

    items = QMimeData()
    items.setData("tableData", create_mime_data(row=1))

    view = controller.widget

    model = controller.sourceModel()
    index = model.index(0, 0)
    assert index.data() == "a"

    event = QDropEvent(QPoint(0, 0), Qt.CopyAction, items,
                       Qt.LeftButton, Qt.NoModifier, QEvent.Drop)
    # Patch the source
    event.source = lambda: controller.widget

    view.dropEvent(event)
    assert index.data() == "b"
    index = model.index(1, 0)
    assert index.data() == "a"

    assert_model_row(controller, 3, "d", True, "CHANGING", 1, 5.0, ["d"])
    assert controller.sourceModel().rowCount(None) == 4

    items.setData("tableData", create_mime_data(row=2))
    # Drag somewhere to outer regions, it will modify last row
    event = QDropEvent(QPoint(0, 1000), Qt.CopyAction, items,
                       Qt.LeftButton, Qt.NoModifier, QEvent.Drop)
    event.source = lambda: controller.widget
    view.dropEvent(event)
    assert controller.sourceModel().rowCount(None) == 4
    assert_model_row(controller, 3, "c", False, "UNKNOWN", 2, 1.0, ["c"])


def test_table_model_view_button_delegate(table_model_view_setup):
    """Test the table button delegate of the karabo table view"""
    clicked = False

    class Delegate(TableButtonDelegate):

        def get_button_text(self, index):
            return "Boolean"

        def isEnabled(self, index=None):
            return index.data(role=Qt.CheckStateRole) == Qt.Checked

        def click_action(self, index):
            nonlocal clicked
            clicked = True

    controller, _, _ = table_model_view_setup

    # XXX: Find a better of testing the delegate!
    button_delegate = Delegate()
    controller.widget.setItemDelegateForColumn(1, button_delegate)
    model = controller.sourceModel()
    idx = model.index(2, 1)
    assert not button_delegate.isEnabled(idx)
    button_delegate.click_action(idx)
    assert clicked

    idx = model.index(0, 1)
    assert button_delegate.isEnabled(idx)
    button_delegate.click_action(idx)


def test_table_model_view_string_button_delegate(gui_app, mocker):
    """Test the table string button delegate of the karabo table view"""
    s = "deviceScene|device_id=XHQ_EG_DG/CAM/CAMERA&name=scene"
    table_hash = Hash(
        "prop", [Hash("arch", s,
                      "bar", "url|https://www.xfel.eu",
                      "foo", True)])

    proxy = get_property_proxy(ButtonObject.getClassSchema(), "prop")
    model = TableElementModel()
    controller = BaseTableController(proxy=proxy, model=model)
    controller.create(None)
    apply_configuration(table_hash, proxy.root_proxy.binding)
    model = controller.sourceModel()

    # 1.1 Device scene delegate
    assert model.rowCount() == 1
    index = model.index(0, 0)
    table_view = controller.tableWidget()
    delegate = table_view.itemDelegate(index)
    assert isinstance(delegate, StringButtonDelegate)
    topology = SystemTopology()
    network = mocker.Mock()
    with singletons(topology=topology, network=network):
        delegate.click_action(index)
        # Click, but device is offline
        network.onExecuteGeneric.assert_not_called()

        device_id = proxy.root_proxy.device_id
        device_hash = Hash(device_id, "")
        device_hash[device_id, ...] = {"status": "ok",
                                       "capabilities": 0,
                                       "host": "BIG_IRON",
                                       "visibility": 4,
                                       "serverId": "swerver",
                                       "classId": "FooBar"}
        topology.initialize(Hash("device", device_hash))
        # Click again, device is online
        delegate.click_action(index)
        args = network.onExecuteGeneric.call_args
        assert "XHQ_EG_DG/CAM/CAMERA" in args[0]
        assert "requestScene" in args[0]

        # 1.2 Check the url web link delegate
        index = model.index(0, 1)
        table_view = controller.tableWidget()
        delegate = table_view.itemDelegate(index)
        assert isinstance(delegate, StringButtonDelegate)
        path = f"{DELEGATES}.webbrowser"
        web = mocker.patch(path)
        delegate.click_action(index)
        web.open_new.assert_called_with("https://www.xfel.eu")

        # Retrieve default scene if no name is provided
        index = model.index(0, 0)
        delegate = table_view.itemDelegate(index)
        s = "deviceScene|device_id=XHQ_EG_DG/CAM/CAMERA"
        table_hash = Hash(
            "prop", [Hash("arch", s,
                          "bar", "url|https://www.xfel.eu",
                          "foo", True)])
        apply_configuration(table_hash, proxy.root_proxy.binding)
        rd = mocker.patch(f"{DELEGATES}.retrieve_default_scene")
        delegate.click_action(index)
        rd.assert_called_with("XHQ_EG_DG/CAM/CAMERA")

    # teardown
    controller.destroy()


def test_table_model_view_set_data(table_model_view_setup):
    controller, proxy, _ = table_model_view_setup
    proxy.edit_value = None
    assert proxy.edit_value is None

    model = controller.sourceModel()
    index = model.index(0, 0)
    assert index.data() == "a"
    model.setData(index, "b")
    index = model.index(0, 0)
    assert index.data() == "b"
    assert proxy.edit_value is not None
    first_hash = proxy.edit_value[0]
    assert first_hash["arch"] == "b"

    index = model.index(0, 3)
    assert index.data() == "1"
    model.setData(index, "13")
    index = model.index(0, 3)
    assert index.data() == "13"
    first_hash = proxy.edit_value[0]
    assert first_hash["cat"] == 13

    index = model.index(0, 4)
    assert index.data() == "2.0"
    model.setData(index, "2.12456")
    index = model.index(0, 4)
    assert index.data() == "2.12456"
    first_hash = proxy.edit_value[0]
    assert first_hash["dog"] == 2.12456

    index = model.index(0, 5)
    assert index.data() == "a"
    model.setData(index, list2string(["a", "b", "c"]))
    index = model.index(0, 5)
    assert index.data() == "a,b,c"
    first_hash = proxy.edit_value[0]
    assert first_hash["eagle"] == ["a", "b", "c"]


def test_table_model_view_basic_settings_controller(table_model_view_setup):
    controller, _, _ = table_model_view_setup
    assert not controller.isReadOnly()
    assert isinstance(controller.widget, KaraboTableView)
    assert controller.getBindings() is not None

    # Destroy before teardown
    controller.destroy()
    assert controller.widget is None
    assert controller.sourceModel() is None


def test_table_model_view_table_actions(table_model_view_setup):
    controller, _, _ = table_model_view_setup
    actions = controller.widget.actions()
    assert len(actions) == 1
    action = actions[0]
    assert action.text() == "Resize To Contents"
    assert not action.isChecked()
    action.trigger()
    assert action.isChecked()


@pytest.fixture
def filter_table_model_view_setup(gui_app):
    proxy = get_property_proxy(Object.getClassSchema(), "prop")
    model = FilterTableElementModel()
    controller = BaseFilterTableController(proxy=proxy, model=model)
    controller.create(None)
    assert controller.isReadOnly()
    controller.set_read_only(False)
    table_hash = Hash(
        "prop",
        [Hash("arch", "a", "foo", True, "bar", "ACTIVE", "cat", 1,
              "dog", 2.0, "eagle", ["a"]),
         Hash("arch", "b", "foo", False, "bar", "ERROR", "cat", 3,
              "dog", 4.0, "eagle", ["b"]),
         Hash("arch", "c", "foo", False, "bar", "UNKNOWN", "cat", 2,
              "dog", 1.0, "eagle", ["c"]),
         Hash("arch", "d", "foo", True, "bar", "CHANGING", "cat", 1,
              "dog", 5.0, "eagle", ["d"])])
    apply_configuration(table_hash, proxy.root_proxy.binding)

    yield controller, proxy, model
    controller.destroy()


def test_filter_table_model_view_set_value(filter_table_model_view_setup):
    controller, _, _ = filter_table_model_view_setup
    assert_model_row(controller, 0, "a", True, "ACTIVE", 1, 2.0, ["a"])
    assert_model_row(controller, 1, "b", False, "ERROR", 3, 4.0, ["b"])
    assert_model_row(controller, 2, "c", False, "UNKNOWN", 2, 1.0, ["c"])
    assert_model_row(controller, 3, "d", True, "CHANGING", 1, 5.0, ["d"])


def test_filter_table_model_view_filter_combo(filter_table_model_view_setup):
    controller, _, _ = filter_table_model_view_setup
    model = controller.sourceModel()
    for i in range(controller.columnCombo.count()):
        header_text = model.headerData(i, orientation=Qt.Horizontal,
                                       role=Qt.DisplayRole)
        assert header_text == controller.columnCombo.itemText(i)
    controller.columnCombo.setCurrentIndex(2)
    controller.searchLabel.setText("ACTIVE")

    filter_model = controller.tableModel()

    # check if its in the filtered view
    index = model.index(0, 2, QModelIndex())
    # ACTIVE is visible
    assert filter_model.mapFromSource(index).isValid()
    index = model.index(1, 2, QModelIndex())
    # row with ERROR cell is not visible
    assert not filter_model.mapFromSource(index).isValid()


def test_filter_table_model_view_table_actions(filter_table_model_view_setup,
                                               mocker):
    controller, _, model = filter_table_model_view_setup
    actions = controller.widget.actions()
    assert len(actions) == 4
    assert actions[0].text() == "Resize To Contents"
    assert actions[1].text() == "Set Default Filter Column"
    assert actions[2].text() == "Sorting Enabled"
    assert actions[3].text() == "Show Filter Column Toggle"

    assert model.filterKeyColumn == 0
    path = "karabogui.controllers.table.controller.QInputDialog"
    dia = mocker.patch(path)
    dia.getInt.return_value = 2, True
    actions[1].trigger()
    assert model.filterKeyColumn == 2

    assert not controller.model.sortingEnabled
    actions[2].trigger()
    assert controller.model.sortingEnabled

    assert not controller.model.showFilterKeyColumn
    actions[3].trigger()
    assert controller.model.showFilterKeyColumn


def test_filter_table_model_view_model_data(filter_table_model_view_setup):
    controller, proxy, _ = filter_table_model_view_setup
    model = controller.tableWidget().model()

    h = Hash(
        "prop",
        [Hash("arch", "a", "foo", True, "bar", "ACTIVE", "cat", 1,
              "dog", 2.0, "eagle", ["a"]),
         Hash("arch", "b", "foo", False, "bar", "ERROR", "cat", 3,
              "dog", 4.0, "eagle", ["b"]),
         Hash("arch", "c", "foo", False, "bar", "UNKNOWN", "cat", 2,
              "dog", 1.0, "eagle", ["c"])])
    apply_configuration(h, proxy.root_proxy.binding)
    assert model.rowCount(QModelIndex()) == 3

    # Test model data
    value = controller.get_model_data(0, 0)
    assert value == ("arch", "a")
    value = controller.get_model_data(0, 1)
    assert value == ("foo", True)
    value = controller.get_model_data(0, 2)
    assert value == ("bar", "ACTIVE")
    value = controller.get_model_data(1, 0)
    assert value == ("arch", "b")


def test_filter_table_model_view_view_key_events(
        filter_table_model_view_setup):
    controller, proxy, _ = filter_table_model_view_setup

    model = controller.tableWidget().model()

    h = Hash(
        "prop",
        [Hash("arch", "a", "foo", True, "bar", "ACTIVE", "cat", 1,
              "dog", 2.0, "eagle", ["a"]),
         Hash("arch", "b", "foo", False, "bar", "ERROR", "cat", 3,
              "dog", 4.0, "eagle", ["b"]),
         Hash("arch", "c", "foo", False, "bar", "UNKNOWN", "cat", 2,
              "dog", 1.0, "eagle", ["c"])])
    apply_configuration(h, proxy.root_proxy.binding)
    assert model.rowCount(QModelIndex()) == 3

    assert not controller.isReadOnly()

    # 1. Move row up and down
    model = controller.sourceModel()
    assert model.index(0, 0).data() == "a"
    assert model.index(1, 0).data() == "b"
    controller.tableWidget().selectRow(1)
    keySequence(controller.tableWidget(), QKeySequence.SelectPreviousLine)
    assert model.index(0, 0).data() == "b"
    assert model.index(1, 0).data() == "a"
    controller.tableWidget().selectRow(0)
    keySequence(controller.tableWidget(), QKeySequence.SelectNextLine)
    assert model.index(0, 0).data() == "a"
    assert model.index(1, 0).data() == "b"

    controller.tableWidget().selectRow(3)
    # 2. Add a row
    keySequence(controller.tableWidget(), QKeySequence.New)
    assert model.rowCount(QModelIndex()) == 4

    # 3. Select last row and delete a row
    controller.tableWidget().selectRow(4)
    keySequence(controller.tableWidget(), QKeySequence.Delete)
    assert model.rowCount(QModelIndex()) == 3


def test_filter_table_model_view_table_controller_extras(gui_app):
    proxy = get_property_proxy(Object.getClassSchema(), "prop")
    model = FilterTableElementModel(sortingEnabled=True)
    controller = BaseFilterTableController(proxy=proxy,
                                           model=model)
    controller.create(None)
    # 1.1 We have sorting enabled
    assert controller.tableWidget().isSortingEnabled()
    # 1.2 Does not have any buttons
    assert not controller._table_buttons
    controller.destroy()

    proxy = get_property_proxy(ButtonObject.getClassSchema(), "prop")
    model = FilterTableElementModel(sortingEnabled=False)
    controller = BaseFilterTableController(proxy=proxy, model=model)
    controller.create(None)
    # 2.1 We have sorting enabled
    assert not controller.tableWidget().isSortingEnabled()
    # 2.2 Does not have any buttons
    assert controller._table_buttons


def test_filter_table_model_view_table_controller_state(gui_app):
    proxy = get_property_proxy(StateObject.getClassSchema(), "prop")
    model = TableElementModel()
    controller = BaseTableController(proxy=proxy, model=model)
    controller.create(None)
    controller.set_read_only(False)
    assert controller.widget.isEnabled()
    set_proxy_value(proxy, "state", "CHANGING")
    assert not controller.widget.isEnabled()
    set_proxy_value(proxy, "state", "ON")
    assert controller.widget.isEnabled()

    # We have buttons and are also disabled
    controller.set_read_only(True)
    set_proxy_value(proxy, "state", "CHANGING")
    assert not controller.widget.isEnabled()
    set_proxy_value(proxy, "state", "ON")
    assert controller.widget.isEnabled()

    with access_level(AccessLevel.OBSERVER):
        set_proxy_value(proxy, "state", "ON")
        assert not controller.widget.isEnabled()

    controller.destroy()
