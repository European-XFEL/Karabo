from unittest import main, mock

import pytest
from qtpy.QtCore import (
    QEvent, QItemSelectionModel, QMimeData, QModelIndex, QPoint, Qt)
from qtpy.QtGui import QDropEvent, QKeySequence

from karabo.common.api import State
from karabo.common.scenemodel.api import (
    FilterTableElementModel, TableElementModel)
from karabo.native import (
    AccessMode, Bool, Configurable, Double, Hash, Int32, MetricPrefix, String,
    Unit, VectorHash, VectorString)
from karabogui.binding.config import apply_configuration
from karabogui.controllers.table.api import (
    BaseFilterTableController, BaseTableController, KaraboTableView,
    StringButtonDelegate, TableButtonDelegate, create_mime_data, list2string)
from karabogui.testing import (
    GuiTestCase, get_property_proxy, keySequence, set_proxy_value, singletons)
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
        rows=TableSchema,
        accessMode=AccessMode.READONLY,
        allowedStates=[State.ON])


class TestTableModelView(GuiTestCase):
    def setUp(self):
        super(TestTableModelView, self).setUp()
        self.proxy = get_property_proxy(Object.getClassSchema(), "prop")
        self.model = TableElementModel()
        self.controller = BaseTableController(proxy=self.proxy,
                                              model=self.model)
        self.controller.create(None)
        self.assertTrue(self.controller.isReadOnly())
        self.controller.set_read_only(False)
        self.table_hash = Hash(
            "prop",
            [Hash("arch", "a", "foo", True, "bar", "ACTIVE", "cat", 1,
                  "dog", 2.0, "eagle", ["a"]),
             Hash("arch", "b", "foo", False, "bar", "ERROR", "cat", 3,
                  "dog", 4.0, "eagle", ["b"]),
             Hash("arch", "c", "foo", False, "bar", "UNKNOWN", "cat", 2,
                  "dog", 1.0, "eagle", ["c"]),
             Hash("arch", "d", "foo", True, "bar", "CHANGING", "cat", 1,
                  "dog", 5.0, "eagle", ["d"])])
        apply_configuration(self.table_hash, self.proxy.root_proxy.binding)

    def tearDown(self):
        self.controller.destroy()

    def assertModelRow(self, row, first_column, second_column, third_column,
                       fourth_column, fifth_column, sixth_column, size=None):
        model = self.controller.sourceModel()
        # Boolean is represented as Qt value in table model

        self.assertEqual(model.index(row, 0).data(), first_column)
        qt_bool = Qt.Checked if second_column else Qt.Unchecked
        self.assertEqual(model.index(row, 1).data(role=Qt.CheckStateRole),
                         qt_bool)
        self.assertEqual(model.index(row, 2).data(), third_column)
        # integer and floating point are string data to outside
        self.assertEqual(model.index(row, 3).data(), str(fourth_column))
        self.assertEqual(model.index(row, 4).data(), str(fifth_column))
        # Vector data is always retrieved as string list
        self.assertEqual(model.index(row, 5).data(), list2string(sixth_column))

        data = self.controller.sourceModel()._data
        # Our Hash still has real booleans, but the model does not!
        self.assertEqual(data[row], Hash("arch", first_column,
                                         "foo", second_column,
                                         "bar", third_column,
                                         "cat", fourth_column,
                                         "dog", fifth_column,
                                         "eagle", sixth_column))
        if size is not None:
            self.assertEqual(data, size)

    def test_focuspolicy(self):
        self.assertEqual(self.controller.widget.focusPolicy(), Qt.StrongFocus)

    def test_deviceId(self):
        deviceId = self.controller.getInstanceId()
        self.assertEqual(deviceId, "TestDevice")

    def test_set_value(self):
        self.assertModelRow(0, "a", True, "ACTIVE", 1, 2.0, ["a"])
        self.assertModelRow(1, "b", False, "ERROR", 3, 4.0, ["b"])
        self.assertModelRow(2, "c", False, "UNKNOWN", 2, 1.0, ["c"])
        self.assertModelRow(3, "d", True, "CHANGING", 1, 5.0, ["d"])

    def test_header(self):
        model = self.controller.sourceModel()
        header_keys = model._header
        self.assertEqual(header_keys, ["arch", "foo", "bar", "cat", "dog",
                                       "eagle"])
        foo = model.headerData(1, orientation=Qt.Horizontal,
                               role=Qt.DisplayRole)
        self.assertEqual(foo, "Foo")
        bar = model.headerData(2, orientation=Qt.Horizontal,
                               role=Qt.DisplayRole)
        self.assertEqual(bar, "Bar")
        cat = model.headerData(3, orientation=Qt.Horizontal,
                               role=Qt.DisplayRole)
        self.assertEqual(cat, "Cat")
        dog = model.headerData(4, orientation=Qt.Horizontal,
                               role=Qt.DisplayRole)
        self.assertEqual(dog, "Dog")
        eagle = model.headerData(5, orientation=Qt.Horizontal,
                                 role=Qt.DisplayRole)
        self.assertEqual(eagle, "Eagle [mm]")
        is_none = model.headerData(1, orientation=Qt.Horizontal,
                                   role=Qt.EditRole)
        self.assertIsNone(is_none)
        is_none = model.headerData(1, orientation=Qt.Vertical,
                                   role=Qt.EditRole)
        self.assertIsNone(is_none)

        v = model.headerData(0, orientation=Qt.Vertical,
                             role=Qt.DisplayRole)
        self.assertEqual(v, "0")

    def test_move_row_up(self):
        self.assertModelRow(0, "a", True, "ACTIVE", 1, 2.0, ["a"])
        self.assertModelRow(1, "b", False, "ERROR", 3, 4.0, ["b"])

        # Move it
        model = self.controller.sourceModel()
        model.move_row_up(1)

        self.assertModelRow(0, "b", False, "ERROR", 3, 4.0, ["b"])
        self.assertModelRow(1, "a", True, "ACTIVE", 1, 2.0, ["a"])

        # --- Corner case ---
        model.move_row_up(0)
        self.assertModelRow(0, "b", False, "ERROR", 3, 4.0, ["b"])
        self.assertModelRow(1, "a", True, "ACTIVE", 1, 2.0, ["a"])

    def test_move_row_down(self):
        self.assertModelRow(0, "a", True, "ACTIVE", 1, 2.0, ["a"])
        self.assertModelRow(1, "b", False, "ERROR", 3, 4.0, ["b"])

        # Move it
        model = self.controller.sourceModel()
        model.move_row_down(0)

        self.assertModelRow(0, "b", False, "ERROR", 3, 4.0, ["b"])
        self.assertModelRow(1, "a", True, "ACTIVE", 1, 2.0, ["a"])
        self.assertModelRow(2, "c", False, "UNKNOWN", 2, 1.0, ["c"])
        self.assertModelRow(3, "d", True, "CHANGING", 1, 5.0, ["d"])

        # --- Corner case ---
        model.move_row_down(3)
        self.assertModelRow(0, "b", False, "ERROR", 3, 4.0, ["b"])
        self.assertModelRow(1, "a", True, "ACTIVE", 1, 2.0, ["a"])
        self.assertModelRow(2, "c", False, "UNKNOWN", 2, 1.0, ["c"])
        self.assertModelRow(3, "d", True, "CHANGING", 1, 5.0, ["d"])

    def test_duplicate_row(self):
        self.assertModelRow(0, "a", True, "ACTIVE", 1, 2.0, ["a"])
        self.assertModelRow(1, "b", False, "ERROR", 3, 4.0, ["b"])
        model = self.controller.sourceModel()
        model.duplicate_row(0)
        self.assertModelRow(0, "a", True, "ACTIVE", 1, 2.0, ["a"])
        self.assertModelRow(1, "a", True, "ACTIVE", 1, 2.0, ["a"])
        self.assertModelRow(2, "b", False, "ERROR", 3, 4.0, ["b"])

    def test_empty_value(self):
        model = self.controller.sourceModel()
        self.assertNotEqual(model._data, [])
        model.clear_model()
        self.assertEqual(model._data, [])
        self.assertEqual(model.rowCount(QModelIndex()), 0)
        # And a new round of applying configuration
        apply_configuration(self.table_hash, self.proxy.root_proxy.binding)
        self.assertEqual(model.rowCount(QModelIndex()), 4)
        # Set empty list
        apply_configuration(Hash("prop", []), self.proxy.root_proxy.binding)
        self.assertEqual(model.rowCount(QModelIndex()), 0)
        # Fill up again and decrease to 3
        apply_configuration(self.table_hash, self.proxy.root_proxy.binding)
        self.assertEqual(model.rowCount(QModelIndex()), 4)

        h = Hash(
            "prop",
            [Hash("arch", "a", "foo", True, "bar", "ACTIVE", "cat", 1,
                  "dog", 2.0, "eagle", ["a"]),
             Hash("arch", "b", "foo", False, "bar", "ERROR", "cat", 3,
                  "dog", 4.0, "eagle", ["b"]),
             Hash("arch", "c", "foo", False, "bar", "UNKNOWN", "cat", 2,
                  "dog", 1.0, "eagle", ["c"])])
        apply_configuration(h, self.proxy.root_proxy.binding)
        self.assertEqual(model.rowCount(QModelIndex()), 3)

        # Test model data
        value = self.controller.get_model_data(0, 0)
        self.assertEqual(value, ("arch", "a"))
        value = self.controller.get_model_data(0, 1)
        self.assertEqual(value, ("foo", True))
        value = self.controller.get_model_data(0, 2)
        self.assertEqual(value, ("bar", "ACTIVE"))

        value = self.controller.get_model_data(1, 0)
        self.assertEqual(value, ("arch", "b"))
        value = self.controller.getModelData(1, 0)
        self.assertEqual(value, ("arch", "b"))

    def test_background_value(self):
        model = self.controller.sourceModel()
        index = model.index(0, 1)
        self.assertEqual(index.data(role=Qt.BackgroundRole), None)
        index = model.index(0, 2)
        brush = index.data(role=Qt.BackgroundRole)
        self.assertNotEqual(brush, None)
        color = brush.color().getRgb()
        self.assertEqual(color, (120, 255, 0, 255))
        # Set new state
        model.setData(index, value="ERROR", role=Qt.EditRole)
        brush = index.data(role=Qt.BackgroundRole)
        color = brush.color().getRgb()
        self.assertEqual(color, (255, 0, 0, 255))

        # Set wrong state
        model.setData(index, value=" NOSTATE", role=Qt.EditRole)
        brush = index.data(role=Qt.BackgroundRole)
        self.assertIsNone(brush)

    def test_set_boolean_data(self):
        model = self.controller.sourceModel()
        index = model.index(0, 1)
        self.assertModelRow(0, "a", True, "ACTIVE", 1, 2.0, ["a"])
        model.setData(index, value=Qt.Unchecked, role=Qt.CheckStateRole)
        self.assertModelRow(0, "a", False, "ACTIVE", 1, 2.0, ["a"])
        model.setData(index, value=Qt.Checked, role=Qt.CheckStateRole)
        self.assertModelRow(0, "a", True, "ACTIVE", 1, 2.0, ["a"])

    def test_set_vector_data(self):
        model = self.controller.sourceModel()
        self.assertModelRow(0, "a", True, "ACTIVE", 1, 2.0, ["a"])
        index = model.index(0, 5)
        # The model expects string data for lists
        model.setData(index, value=list2string(["a", "b", "c"]))
        data = index.data(role=Qt.DisplayRole)
        self.assertEqual(data, "a,b,c")
        with pytest.raises(AttributeError):
            # List does not have split attribute ...
            model.setData(index, value=["a", "b", "c"])

    def test_tooltip_data(self):
        model = self.controller.sourceModel()
        index = model.index(0, 5)
        data = index.data(role=Qt.ToolTipRole)
        self.assertEqual(data, "a")
        ret = model.setData(index, "Not allowed", role=Qt.ToolTipRole)
        self.assertFalse(ret)

        # Check the header
        data = model.headerData(section=5, orientation=Qt.Horizontal,
                                role=Qt.ToolTipRole)
        self.assertIn("metricPrefixSymbol", data)
        self.assertIn("unitSymbol", data)
        self.assertIn("defaultValue", data)
        self.assertIn("valueType", data)
        self.assertIn("VECTOR_STRING", data)
        self.assertIn("description", data)
        self.assertIn("This is a description for a table", data)
        self.assertNotIn("minExc", data)
        self.assertNotIn("minInc", data)

    def test_invalid_model_index(self):
        model = self.controller.sourceModel()
        ret = model.setData(QModelIndex(), 2, Qt.EditRole)
        self.assertFalse(ret)
        self.assertIsNone(model.data(QModelIndex()))
        self.assertEqual(model.flags(QModelIndex()), 0)

    def test_model_flags(self):
        model = self.controller.sourceModel()
        # 1. User checkable boolean
        index = model.index(0, 1)
        flags = index.flags()
        self.assertEqual(flags & Qt.ItemIsUserCheckable,
                         Qt.ItemIsUserCheckable)
        self.assertNotEqual(flags & Qt.ItemIsEditable,
                            Qt.ItemIsEditable)

        # 2. Readonly string
        index = model.index(0, 2)
        flags = index.flags()
        self.assertNotEqual(flags & Qt.ItemIsUserCheckable,
                            Qt.ItemIsUserCheckable)
        self.assertNotEqual(flags & Qt.ItemIsEditable, Qt.ItemIsEditable)

        # 3. reconfigurable int
        index = model.index(0, 3)
        flags = index.flags()
        self.assertNotEqual(flags & Qt.ItemIsUserCheckable,
                            Qt.ItemIsUserCheckable)
        self.assertEqual(flags & Qt.ItemIsEditable, Qt.ItemIsEditable)

    def test_read_only_table(self):
        self.controller.set_read_only(True)

    def test_view_menu_select(self):
        menu_path = "karabogui.controllers.table.controller.QMenu"
        pos = QPoint(0, 0)
        with mock.patch(menu_path) as menu:
            model = self.controller.sourceModel()
            index = model.index(2, 0)

            view = self.controller.widget
            selection = view.selectionModel()
            selection.setCurrentIndex(index,
                                      QItemSelectionModel.ClearAndSelect)
            # Assert the selection
            self.assertEqual(view.currentIndex(), index)
            self.assertEqual(self.controller.currentIndex(), index)

            self.controller._context_menu(pos)
            menu.assert_called_once()

            model.setData(index, "Yes A String")
            self.assertEqual(index.data(), "Yes A String")
            self.controller._set_index_default()
            self.assertEqual(index.data(), "NoString")

            # Invalid index also provides a model!
            menu.reset_mock()
            selection.setCurrentIndex(QModelIndex(),
                                      QItemSelectionModel.ClearAndSelect)
            self.assertEqual(view.currentIndex(), QModelIndex())

            self.controller._context_menu(pos)
            menu.assert_called_once()

    def test_drag_drop_view(self):
        """Test the drag & drop of the table view"""
        items = QMimeData()
        items.setData("tableData", create_mime_data(row=1))

        view = self.controller.widget

        model = self.controller.sourceModel()
        index = model.index(0, 0)
        self.assertEqual(index.data(), "a")

        event = QDropEvent(QPoint(0, 0), Qt.CopyAction, items,
                           Qt.LeftButton, Qt.NoModifier, QEvent.Drop)
        # Patch the source
        event.source = lambda: self.controller.widget

        view.dropEvent(event)
        self.assertEqual(index.data(), "b")
        index = model.index(1, 0)
        self.assertEqual(index.data(), "a")

        self.assertModelRow(3, "d", True, "CHANGING", 1, 5.0, ["d"])
        self.assertEqual(self.controller.sourceModel().rowCount(None), 4)

        items.setData("tableData", create_mime_data(row=2))
        # Drag somewhere to outer regions, it will modify last row
        event = QDropEvent(QPoint(0, 1000), Qt.CopyAction, items,
                           Qt.LeftButton, Qt.NoModifier, QEvent.Drop)
        event.source = lambda: self.controller.widget
        view.dropEvent(event)
        self.assertEqual(self.controller.sourceModel().rowCount(None), 4)
        self.assertModelRow(3, "c", False, "UNKNOWN", 2, 1.0, ["c"])

    def test_button_delegate(self):
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

        # XXX: Find a better of testing the delegate!
        button_delegate = Delegate()
        self.controller.widget.setItemDelegateForColumn(1, button_delegate)
        model = self.controller.sourceModel()
        index = model.index(2, 1)
        self.assertFalse(button_delegate.isEnabled(index))
        button_delegate.click_action(index)
        self.assertTrue(clicked)

        index = model.index(0, 1)
        self.assertTrue(button_delegate.isEnabled(index))
        button_delegate.click_action(index)

    def test_string_button_delegate(self):
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
        self.assertEqual(model.rowCount(), 1)
        index = model.index(0, 0)
        table_view = controller.tableWidget()
        delegate = table_view.itemDelegate(index)
        self.assertIsInstance(delegate, StringButtonDelegate)
        topology = SystemTopology()
        network = mock.Mock()
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
            self.assertIsInstance(delegate, StringButtonDelegate)
            path = f"{DELEGATES}.webbrowser"
            with mock.patch(path) as web:
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
            with mock.patch(f"{DELEGATES}.retrieve_default_scene") as rd:
                delegate.click_action(index)
                rd.assert_called_with('XHQ_EG_DG/CAM/CAMERA')

        controller.destroy()

    def test_set_data(self):
        self.proxy.edit_value = None
        self.assertIsNone(self.proxy.edit_value)

        model = self.controller.sourceModel()
        index = model.index(0, 0)
        self.assertEqual(index.data(), "a")
        model.setData(index, "b")
        index = model.index(0, 0)
        self.assertEqual(index.data(), "b")
        self.assertIsNotNone(self.proxy.edit_value)
        first_hash = self.proxy.edit_value[0]
        self.assertEqual(first_hash["arch"], "b")

        index = model.index(0, 3)
        self.assertEqual(index.data(), "1")
        model.setData(index, "13")
        index = model.index(0, 3)
        self.assertEqual(index.data(), "13")
        first_hash = self.proxy.edit_value[0]
        self.assertEqual(first_hash["cat"], 13)

        index = model.index(0, 4)
        self.assertEqual(index.data(), "2.0")
        model.setData(index, "2.12456")
        index = model.index(0, 4)
        self.assertEqual(index.data(), "2.12456")
        first_hash = self.proxy.edit_value[0]
        self.assertEqual(first_hash["dog"], 2.12456)

        index = model.index(0, 5)
        self.assertEqual(index.data(), "a")
        model.setData(index, list2string(["a", "b", "c"]))
        index = model.index(0, 5)
        self.assertEqual(index.data(), "a,b,c")
        first_hash = self.proxy.edit_value[0]
        self.assertEqual(first_hash["eagle"], ["a", "b", "c"])

    def test_basic_settings_controller(self):
        self.assertFalse(self.controller.isReadOnly())
        self.assertIsInstance(self.controller.widget, KaraboTableView)
        self.assertIsNotNone(self.controller.getBindings())

        # Destroy before teardown
        self.controller.destroy()
        self.assertIsNone(self.controller.widget)
        self.assertIsNone(self.controller.sourceModel())

    def test_table_actions(self):
        actions = self.controller.widget.actions()
        self.assertEqual(len(actions), 1)
        action = actions[0]
        self.assertEqual(action.text(), "Resize To Contents")
        self.assertFalse(action.isChecked())
        action.trigger()
        self.assertTrue(action.isChecked())


class TestFilterTableModelView(GuiTestCase):
    def setUp(self):
        super().setUp()
        self.proxy = get_property_proxy(Object.getClassSchema(), "prop")
        self.model = FilterTableElementModel()
        self.controller = BaseFilterTableController(proxy=self.proxy,
                                                    model=self.model)
        self.controller.create(None)
        self.assertTrue(self.controller.isReadOnly())
        self.controller.set_read_only(False)
        self.table_hash = Hash(
            "prop",
            [Hash("arch", "a", "foo", True, "bar", "ACTIVE", "cat", 1,
                  "dog", 2.0, "eagle", ["a"]),
             Hash("arch", "b", "foo", False, "bar", "ERROR", "cat", 3,
                  "dog", 4.0, "eagle", ["b"]),
             Hash("arch", "c", "foo", False, "bar", "UNKNOWN", "cat", 2,
                  "dog", 1.0, "eagle", ["c"]),
             Hash("arch", "d", "foo", True, "bar", "CHANGING", "cat", 1,
                  "dog", 5.0, "eagle", ["d"])])
        apply_configuration(self.table_hash, self.proxy.root_proxy.binding)

    def tearDown(self):
        self.controller.destroy()

    def assertModelRow(self, row, first_column, second_column, third_column,
                       fourth_column, fifth_column, sixth_column, size=None):
        model = self.controller.sourceModel()
        # Boolean is represented as Qt value in table model

        self.assertEqual(model.index(row, 0).data(), first_column)
        qt_bool = Qt.Checked if second_column else Qt.Unchecked
        self.assertEqual(model.index(row, 1).data(role=Qt.CheckStateRole),
                         qt_bool)
        self.assertEqual(model.index(row, 2).data(), third_column)
        # integer and floating point are string data to outside
        self.assertEqual(model.index(row, 3).data(), str(fourth_column))
        self.assertEqual(model.index(row, 4).data(), str(fifth_column))
        # Vector data is always retrieved as string list
        self.assertEqual(model.index(row, 5).data(), list2string(sixth_column))

        data = self.controller.sourceModel()._data
        # Our Hash still has real booleans, but the model does not!
        self.assertEqual(data[row], Hash("arch", first_column,
                                         "foo", second_column,
                                         "bar", third_column,
                                         "cat", fourth_column,
                                         "dog", fifth_column,
                                         "eagle", sixth_column))
        if size is not None:
            self.assertEqual(data, size)

    def test_set_value(self):
        self.assertModelRow(0, "a", True, "ACTIVE", 1, 2.0, ["a"])
        self.assertModelRow(1, "b", False, "ERROR", 3, 4.0, ["b"])
        self.assertModelRow(2, "c", False, "UNKNOWN", 2, 1.0, ["c"])
        self.assertModelRow(3, "d", True, "CHANGING", 1, 5.0, ["d"])

    def test_table_actions(self):
        actions = self.controller.widget.actions()
        self.assertEqual(len(actions), 4)
        self.assertEqual(actions[0].text(), "Resize To Contents")
        self.assertEqual(actions[1].text(), "Set Default Filter Column")
        self.assertEqual(actions[2].text(), "Sorting Enabled")
        self.assertEqual(actions[3].text(), "Show Filter Column Toggle")

        self.assertEqual(self.model.filterKeyColumn, 0)
        path = "karabogui.controllers.table.controller.QInputDialog"
        with mock.patch(path) as dia:
            dia.getInt.return_value = 2, True
            actions[1].trigger()
            self.assertEqual(self.model.filterKeyColumn, 2)

        self.assertFalse(self.controller.model.sortingEnabled)
        actions[2].trigger()
        self.assertTrue(self.controller.model.sortingEnabled)

        self.assertFalse(self.controller.model.showFilterKeyColumn)
        actions[3].trigger()
        self.assertTrue(self.controller.model.showFilterKeyColumn)

    def test_model_data(self):
        model = self.controller.tableWidget().model()

        h = Hash(
            "prop",
            [Hash("arch", "a", "foo", True, "bar", "ACTIVE", "cat", 1,
                  "dog", 2.0, "eagle", ["a"]),
             Hash("arch", "b", "foo", False, "bar", "ERROR", "cat", 3,
                  "dog", 4.0, "eagle", ["b"]),
             Hash("arch", "c", "foo", False, "bar", "UNKNOWN", "cat", 2,
                  "dog", 1.0, "eagle", ["c"])])
        apply_configuration(h, self.proxy.root_proxy.binding)
        self.assertEqual(model.rowCount(QModelIndex()), 3)

        # Test model data
        value = self.controller.get_model_data(0, 0)
        self.assertEqual(value, ("arch", "a"))
        value = self.controller.get_model_data(0, 1)
        self.assertEqual(value, ("foo", True))
        value = self.controller.get_model_data(0, 2)
        self.assertEqual(value, ("bar", "ACTIVE"))
        value = self.controller.get_model_data(1, 0)
        self.assertEqual(value, ("arch", "b"))

    def test_view_key_events(self):
        model = self.controller.tableWidget().model()

        h = Hash(
            "prop",
            [Hash("arch", "a", "foo", True, "bar", "ACTIVE", "cat", 1,
                  "dog", 2.0, "eagle", ["a"]),
             Hash("arch", "b", "foo", False, "bar", "ERROR", "cat", 3,
                  "dog", 4.0, "eagle", ["b"]),
             Hash("arch", "c", "foo", False, "bar", "UNKNOWN", "cat", 2,
                  "dog", 1.0, "eagle", ["c"])])
        apply_configuration(h, self.proxy.root_proxy.binding)
        self.assertEqual(model.rowCount(QModelIndex()), 3)

        assert not self.controller.isReadOnly()
        self.controller.tableWidget().selectRow(3)
        # 1. Add a row
        keySequence(self.controller.tableWidget(), QKeySequence.New)
        self.assertEqual(model.rowCount(QModelIndex()), 4)

        # 2. Select last row and delete a row
        self.controller.tableWidget().selectRow(4)
        keySequence(self.controller.tableWidget(), QKeySequence.Delete)
        self.assertEqual(model.rowCount(QModelIndex()), 3)

        # 3. Move row up and down
        model = self.controller.sourceModel()
        assert model.index(0, 0).data() == "a"
        assert model.index(1, 0).data() == "b"
        self.controller.tableWidget().selectRow(1)
        keySequence(self.controller.tableWidget(),
                    QKeySequence.SelectPreviousLine)
        assert model.index(0, 0).data() == "b"
        assert model.index(1, 0).data() == "a"
        self.controller.tableWidget().selectRow(0)
        keySequence(self.controller.tableWidget(),
                    QKeySequence.SelectNextLine)
        assert model.index(0, 0).data() == "a"
        assert model.index(1, 0).data() == "b"

    def test_table_controller_extras(self):
        proxy = get_property_proxy(Object.getClassSchema(), "prop")
        model = FilterTableElementModel(sortingEnabled=True)
        controller = BaseFilterTableController(proxy=proxy,
                                               model=model)
        controller.create(None)
        # 1.1 We have sorting enabled
        self.assertTrue(controller.tableWidget().isSortingEnabled())
        # 1.2 Does not have any buttons
        self.assertFalse(controller._table_buttons)
        controller.destroy()

        proxy = get_property_proxy(ButtonObject.getClassSchema(), "prop")
        model = FilterTableElementModel(sortingEnabled=False)
        controller = BaseFilterTableController(proxy=proxy,
                                               model=model)
        controller.create(None)
        # 2.1 We have sorting enabled
        self.assertFalse(controller.tableWidget().isSortingEnabled())
        # 2.2 Does not have any buttons
        self.assertTrue(controller._table_buttons)

    def test_table_controller_state(self):
        proxy = get_property_proxy(StateObject.getClassSchema(), "prop")
        model = TableElementModel()
        controller = BaseTableController(proxy=proxy, model=model)
        controller.create(None)
        controller.set_read_only(False)
        self.assertTrue(controller.widget.isEnabled())
        set_proxy_value(proxy, "state", "CHANGING")
        self.assertFalse(controller.widget.isEnabled())
        set_proxy_value(proxy, "state", "ON")
        self.assertTrue(controller.widget.isEnabled())

        controller.set_read_only(True)
        set_proxy_value(proxy, "state", "CHANGING")
        self.assertTrue(controller.widget.isEnabled())

        controller.destroy()


if __name__ == "__main__":
    main()
