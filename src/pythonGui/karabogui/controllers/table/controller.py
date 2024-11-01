#############################################################################
# Author: <dennis.goeries@xfel.eu>
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
#############################################################################

from qtpy.QtCore import QModelIndex, Qt
from qtpy.QtWidgets import (
    QAbstractItemView, QAction, QComboBox, QDialog, QHBoxLayout, QHeaderView,
    QInputDialog, QLayout, QLineEdit, QMenu, QPushButton, QVBoxLayout, QWidget)
from traits.api import Bool, Dict, Instance, Type, Undefined, WeakRef

import karabogui.icons as icons
from karabo.common.api import KARABO_SCHEMA_MAX_SIZE, KARABO_SCHEMA_MIN_SIZE
from karabogui.binding.api import (
    StringBinding, get_default_value, get_editor_value)
from karabogui.controllers.api import (
    BaseBindingController, has_options, is_proxy_allowed)
from karabogui.dialogs.api import TopologyDeviceDialog
from karabogui.util import SignalBlocker

from .delegates import (
    TableButtonDelegate, get_display_delegate, get_table_delegate)
from .filter_model import TableSortFilterModel
from .model import TableModel
from .utils import is_writable_binding
from .view import KaraboTableView


class BaseTableController(BaseBindingController):
    """The BaseTableController that can be used for subclassing Tables

    This class provides a read only and reconfigurable version of the table
    element.
    """
    # The scene model class used by this controller
    model = Undefined
    # Does the controller have an own menu. Subclass method `custom_menu`
    # if this is set to `True`
    hasCustomMenu = Bool(False)
    # Overwrite tableModel to specify a custom type of `TableModel`
    tableModelClass = Type(TableModel)

    # Internal traits
    _bindings = Dict
    _readonly = Bool(True)
    _item_model = WeakRef(TableModel, allow_none=True)
    _table_widget = WeakRef(KaraboTableView)
    _table_buttons = Bool(False)

    _hasResize = Bool(False)

    # ---------------------------------------------------------------------
    # Abstract Methods

    def create_widget(self, parent):
        assert self.model is not Undefined

        table_widget = KaraboTableView(parent=parent)
        table_widget.setSelectionBehavior(QAbstractItemView.SelectItems
                                          | QAbstractItemView.SelectRows)
        table_widget.horizontalHeader().setStretchLastSection(True)
        # Create an internal weak ref to subclass create widget
        self._table_widget = table_widget

        # Not every table model might have the `resizeToContents` setting
        model = self.model
        self._hasResize = "resizeToContents" in model.copyable_trait_names()
        if self._hasResize:
            resize_action = QAction("Resize To Contents", table_widget)
            resize_action.triggered.connect(self._resize_contents)
            resize_action.setCheckable(True)
            resize_action.setChecked(self.model.resizeToContents)
            table_widget.addAction(resize_action)

        return table_widget

    def set_read_only(self, ro):
        widget = self._table_widget
        if ro:
            self._readonly = True
            widget.setEditTriggers(QAbstractItemView.NoEditTriggers)
            widget.setAlternatingRowColors(True)
            if self.hasCustomMenu:
                widget.setContextMenuPolicy(Qt.CustomContextMenu)
                widget.customContextMenuRequested.connect(self._custom_menu)
                widget.setSelectionMode(QAbstractItemView.SingleSelection)
            else:
                widget.setSelectionMode(QAbstractItemView.NoSelection)
        else:
            self._readonly = False
            widget.setSelectionMode(QAbstractItemView.SingleSelection)
            widget.setSelectionBehavior(QAbstractItemView.SelectRows)
            flags = (QAbstractItemView.DoubleClicked
                     | QAbstractItemView.AnyKeyPressed
                     | QAbstractItemView.SelectedClicked)
            widget.setEditTriggers(flags)
            widget.setContextMenuPolicy(Qt.CustomContextMenu)
            # A reconfigurable table element always has a menu
            handler = (self._custom_menu if self.hasCustomMenu
                       else self._context_menu)
            widget.customContextMenuRequested.connect(handler)
            widget.setAcceptDrops(True)
        widget.set_read_only(self._readonly)
        widget.setFocusPolicy(Qt.NoFocus if ro else Qt.StrongFocus)
        if self._item_model is not None:
            self._item_model.set_readonly(self._readonly)
            # We must set the delegates firstly on readOnly information!
            self.create_delegates()

    def binding_update(self, proxy):
        binding = proxy.binding
        if binding is not None:
            has_schema = not binding.row_schema.empty()
            if has_schema:
                self._set_bindings(binding)
                if self._hasResize:
                    self._set_table_resize_mode(self.model)

    def value_update(self, proxy):
        value = get_editor_value(proxy, [])
        if not value:
            self._item_model.clear_model()
            return

        # Remove or add rows if necessary
        row_count = self._item_model.rowCount()
        if row_count > len(value):
            start = len(value) - 1
            count = row_count - len(value)
            self._item_model.removeRows(start, count, QModelIndex(),
                                        from_device=True)
        elif row_count < len(value):
            start = row_count
            count = len(value) - row_count
            self._item_model.insertRows(start, count, QModelIndex(),
                                        from_device=True)

        self._item_model.updateData(value)

    def state_update(self, proxy):
        """A change in a state update will only disable reconfigurable tables
        """
        if not self._readonly or self._table_buttons:
            enable = is_proxy_allowed(proxy)
            self.widget.setEnabled(enable)

    def destroy_widget(self):
        if self._item_model is not None:
            self._item_model.setParent(None)
            self._item_model = None
        if self.widget:
            self.widget.setParent(None)
            self.widget = None

    def setEnabled(self, enable):
        """A change in access level happens is forward to the widget

        If the table has table buttons, we always enable or disable
        """
        if self._table_buttons:
            self.widget.setEnabled(enable)
        else:
            super().setEnabled(enable)

    # ---------------------------------------------------------------------
    # Subclass Methods

    def create_delegates(self):
        """Subclass method to set the table delegates in the table element"""
        self.setTableDelegates({})

    def custom_menu(self, pos):
        """Subclass method for own custom menu if ``hasCustomMenu`` is `True`

        :param: pos: The position of the context menu event
        """

    def createModel(self, item_model):
        """Subclass the `createModel` to create a filter model

        Note: This method will fade out and stays for backward compatibility
        """
        return self.createFilterModel(item_model)

    def createFilterModel(self, item_model):
        """Subclass the `createFilterModel` to create a filter model

        Note: Future use with 2.15
        """
        return item_model

    # ---------------------------------------------------------------------
    # Public interface

    def setTableDelegates(self, delegates):
        """Set all table delegates on the table controller

        If no delegates are specified for a column, a delegate is derived
        from the column binding. Use this method in `create_delegates`.

        :param delegates: Dictionary {column: delegate}

        Note: This method was added with Karabo 2.16.X
        """
        assert isinstance(delegates, dict)

        bindings = self._bindings
        keys = bindings.keys()
        get_delegate = (get_display_delegate if self._readonly
                        else get_table_delegate)
        self._table_buttons = False
        for column, key in enumerate(keys):
            delegate = delegates.get(column)
            if delegate is None:
                binding = bindings[key]
                delegate = get_delegate(self.proxy, binding,
                                        self._table_widget)
            if isinstance(delegate, TableButtonDelegate):
                self._table_buttons = True
            self._table_widget.setItemDelegateForColumn(column, delegate)

    def getModelData(self, row, column):
        """Get data from the sourceModel with `row` and `column`

        :param row: row of the data
        :param column: column of the data

        Note: Function added with Karabo > 2.13.X

        Method changed in 2.16.X to account for filtering.
        Please use `get_model_data` in the future.
        """
        return self.tableModel().get_model_data(row, column)

    get_model_data = getModelData

    def getInstanceId(self):
        """Retrieve the `instanceId` of the root proxy of this controller

        Note: Function added with Karabo > 2.13.X
        """
        return self.proxy.root_proxy.device_id

    def columnIndex(self, key):
        """Retrieve the column index for a `key` in the schema

        Note: Function added with Karabo 2.16.X
        """
        bindings = list(self._bindings.keys())
        try:
            return bindings.index(key)
        except ValueError:
            return None

    def columnKey(self, column):
        """Retrieve the schema key for a `column` index

        Note: Function added with Karabo 2.16.X
        """
        bindings = list(self._bindings.keys())
        try:
            return bindings[column]
        except IndexError:
            return None

    def currentIndex(self):
        """Convenience method to get the currentIndex of the selection"""
        return self._table_widget.selectionModel().currentIndex()

    def tableModel(self):
        """Return the `model` of the table controller

        Note: Added with Karabo 2.16.X
        """
        return self._table_widget.model()

    def sourceModel(self):
        """Return the `sourceModel` of the table controller"""
        return self._item_model

    def tableWidget(self):
        """Return the table widget of the controller"""
        return self._table_widget

    def getBindings(self):
        """Access for table bindings of the controller"""
        return self._bindings

    def isReadOnly(self):
        """Access for readOnly property of the table controller"""
        return self._readonly

    # ---------------------------------------------------------------------
    # Controller and Actions Slots (Public)

    def add_row(self):
        row = self.currentIndex().row()
        self._item_model.add_row(row)

    def add_row_below(self):
        row = self.currentIndex().row() + 1
        self._item_model.add_row(row)

    def duplicate_row(self):
        row = self.currentIndex().row()
        self._item_model.duplicate_row(row)

    def move_row_up(self):
        row = self.currentIndex().row()
        self._item_model.move_row_up(row)
        self._table_widget.selectRow(row - 1)

    def move_row_down(self):
        row = self.currentIndex().row()
        self._item_model.move_row_down(row)
        self._table_widget.selectRow(row + 1)

    def remove_row(self):
        index = self.currentIndex()
        self._item_model.removeRows(index.row(), 1, QModelIndex())

    def get_basic_menu(self):
        """Used by subclassed controller to get the basic menu

        Note: Added in Karabo 2.16.X
        """
        menu = QMenu(parent=self._table_widget)
        if self.isReadOnly():
            return menu

        index = self.currentIndex()
        if index.isValid():
            key = list(self._bindings.keys())[index.column()]
            binding = self._bindings[key]
            if is_writable_binding(binding):
                set_default_action = menu.addAction("Set Cell Default")
                set_default_action.triggered.connect(self._set_index_default)
                menu.addSeparator()

            up_action = menu.addAction(icons.arrowFancyUp, "Move Row Up")
            up_action.triggered.connect(self.move_row_up)
            down_action = menu.addAction(icons.arrowFancyDown, "Move Row Down")
            down_action.triggered.connect(self.move_row_down)
            menu.addSeparator()

            add_action = menu.addAction(icons.add, "Add Row below")
            add_action.triggered.connect(self.add_row_below)
            du_action = menu.addAction(icons.editCopy, "Duplicate Row below")
            du_action.triggered.connect(self.duplicate_row)

            remove_action = menu.addAction(icons.delete, "Delete Row")
            remove_action.triggered.connect(self.remove_row)

            # Check for min and max size of the table
            row_count = self._item_model.rowCount()
            attributes = self.proxy.binding.attributes
            min_size = attributes.get(KARABO_SCHEMA_MIN_SIZE)
            max_size = attributes.get(KARABO_SCHEMA_MAX_SIZE)
            add_row = True if max_size is None else row_count + 1 <= max_size
            rm_row = True if min_size is None else row_count - 1 >= min_size

            # Set actions enabled or disabled!
            num_row = self.tableWidget().model().rowCount() - 1
            up_action.setEnabled(index.row() > 0)
            down_action.setEnabled(index.row() < num_row)
            add_action.setEnabled(add_row)
            du_action.setEnabled(add_row)
            remove_action.setEnabled(rm_row)

            if (is_writable_binding(binding, StringBinding)
                    and not has_options(binding)):
                menu.addSeparator()
                device_action = menu.addAction(
                    icons.deviceInstance, "Set Topology DeviceId")
                device_action.triggered.connect(self._device_action)

        else:
            add_action = menu.addAction(icons.add, "Add Row below")
            add_action.triggered.connect(self.add_row_below)

        return menu

    # ---------------------------------------------------------------------
    # Action Slots - Private

    def _device_action(self):
        model = self.tableWidget().model()
        index = model.index_ref(self.currentIndex())
        dialog = TopologyDeviceDialog(parent=self.widget)
        if dialog.exec() == QDialog.Accepted:
            device = dialog.device_id
            self.sourceModel().setData(index, device)

    def _set_index_default(self):
        model = self.tableWidget().model()
        index = model.index_ref(self.currentIndex())
        key = list(self._bindings.keys())[index.column()]
        binding = self._bindings[key]
        default_value = get_default_value(binding, force=True)
        self._item_model.setData(index, default_value, role=Qt.EditRole)

    def _resize_contents(self):
        self.model.resizeToContents = not self.model.resizeToContents
        self._set_table_resize_mode(self.model)

    def _set_table_resize_mode(self, model):
        mode = (QHeaderView.ResizeToContents if model.resizeToContents
                else QHeaderView.Interactive)

        length = len(self.getBindings().keys())
        for index in range(length - 1):
            self._table_widget.horizontalHeader().setSectionResizeMode(
                index, mode)
        self._table_widget.horizontalHeader().setStretchLastSection(True)

    def _context_menu(self, pos):
        """The custom context menu of a reconfigurable table element"""
        selection_model = self._table_widget.selectionModel()
        if selection_model is None:
            # Note: We did not yet receive a schema and thus have no table and
            # selection model!
            return

        menu = self.get_basic_menu()
        menu.exec(self._table_widget.viewport().mapToGlobal(pos))

    # Private interface
    # ---------------------------------------------------------------------

    def _on_user_edit(self, data):
        """Callback method used by `self._item_model` when data changes"""
        if self.proxy.binding is None:
            return
        self.proxy.edit_value = data

    def _set_bindings(self, binding):
        """Configure the column schema hashes and keys

        The schema must not be `None` and is protected when calling this func.
        """
        if self._item_model is not None:
            self._item_model.setParent(None)
            self._item_model = None

        self._bindings = binding.bindings

        source_model = self.tableModelClass(binding, self._on_user_edit,
                                            self._table_widget)
        self._item_model = source_model
        self._item_model.set_readonly(self._readonly)
        model = self.createModel(self._item_model)
        self._table_widget.setModel(model)
        self.create_delegates()

    def _custom_menu(self, pos):
        """Private method to direct to the custom context menu"""
        selection_model = self._table_widget.selectionModel()
        if selection_model is None:
            return

        return self.custom_menu(pos)


# --------------------------------------------------------------------------
# Filter Table Controller


class BaseFilterTableController(BaseTableController):
    searchLabel = Instance(QLineEdit)
    columnCombo = Instance(QComboBox)

    def create_widget(self, parent):
        table_widget = super().create_widget(parent)
        widget = QWidget(parent)
        widget.addActions(table_widget.actions())

        widget_layout = QVBoxLayout()
        widget_layout.setContentsMargins(0, 0, 0, 0)
        widget_layout.setSizeConstraint(QLayout.SetNoConstraint)

        hor_layout = QHBoxLayout()
        hor_layout.setContentsMargins(0, 0, 0, 0)
        hor_layout.setSizeConstraint(QLayout.SetNoConstraint)

        self.columnCombo = QComboBox(widget)
        self.columnCombo.setVisible(False)
        self.columnCombo.setToolTip("Active search column")

        self.searchLabel = QLineEdit(widget)
        self.searchLabel.setToolTip(
            f"Persisted search column: {self.model.filterKeyColumn}")

        clear_button = QPushButton("Clear Filter", parent=widget)
        clear_button.clicked.connect(self.searchLabel.clear)

        hor_layout.addWidget(self.columnCombo)
        hor_layout.addWidget(self.searchLabel)
        hor_layout.addWidget(clear_button)

        # Complete widget layout and return widget
        widget_layout.addLayout(hor_layout)
        widget_layout.addWidget(table_widget)
        widget.setLayout(widget_layout)

        ac_column_filter = QAction("Set Default Filter Column", table_widget)
        ac_column_filter.triggered.connect(self._change_filter_column)
        widget.addAction(ac_column_filter)

        # Widgets in extensions might not have this model setting
        traits_names = self.model.copyable_trait_names()
        if "sortingEnabled" in traits_names:
            enabled = self.model.sortingEnabled
            if enabled:
                table_widget.setSortingEnabled(True)
                table_widget.sortByColumn(0, Qt.AscendingOrder)

            ac_sort = QAction("Sorting Enabled", table_widget)
            ac_sort.setCheckable(True)
            ac_sort.setChecked(enabled)
            ac_sort.triggered.connect(self._change_sorting_enabled)
            widget.addAction(ac_sort)

        if "showFilterKeyColumn" in traits_names:
            enabled = self.model.showFilterKeyColumn
            self.columnCombo.setVisible(enabled)
            ac_show = QAction("Show Filter Column Toggle", table_widget)
            ac_show.setCheckable(True)
            ac_show.setChecked(enabled)
            ac_show.triggered.connect(self._change_show_combo)
            widget.addAction(ac_show)

        return widget

    def createFilterModel(self, item_model):
        """Create the filter model for the table"""
        filter_model = TableSortFilterModel(self.tableWidget())
        filter_model.setSourceModel(item_model)
        key = self.model.filterKeyColumn
        filter_model.setFilterKeyColumn(key)
        filter_model.setFilterRole(Qt.DisplayRole)
        filter_model.setFilterCaseSensitivity(False)
        filter_model.setFilterFixedString("")
        self.searchLabel.textChanged.connect(filter_model.setFilterFixedString)
        self.columnCombo.currentIndexChanged.connect(
            filter_model.setFilterKeyColumn)

        return filter_model

    def binding_update(self, proxy):
        super().binding_update(proxy)
        model = self.tableModel()
        columns = [model.headerData(col, Qt.Horizontal, Qt.DisplayRole)
                   for col in range(model.columnCount())]
        with SignalBlocker(self.columnCombo):
            self.columnCombo.clear()
            self.columnCombo.addItems(columns)
            self.columnCombo.setCurrentIndex(self.model.filterKeyColumn)

    def _change_filter_column(self):
        max_col = len(self.getBindings())
        column, ok = QInputDialog.getInt(
            self.widget, "Filter Column", "Filter Key Column:",
            value=self.model.filterKeyColumn,
            min=0, max=max_col)
        if ok:
            self.model.filterKeyColumn = column
            filter_model = self.tableWidget().model()
            filter_model.setFilterKeyColumn(column)
            self.searchLabel.setToolTip(f"Persisted search column: {column}")
            with SignalBlocker(self.columnCombo):
                self.columnCombo.setCurrentIndex(column)

    def _change_sorting_enabled(self):
        enabled = not self.model.sortingEnabled
        self.model.sortingEnabled = enabled
        self.tableWidget().setSortingEnabled(enabled)
        if enabled:
            self.tableWidget().sortByColumn(0, Qt.AscendingOrder)

    def _change_show_combo(self):
        enabled = not self.model.showFilterKeyColumn
        self.model.showFilterKeyColumn = enabled
        self.columnCombo.setVisible(enabled)

    # ------------------------------------------------------------------------
    # Action interface

    def sourceRow(self):
        """Method to retrieve the selected row of the source model"""
        index = self.currentIndex()
        return self.tableModel().mapToSource(index).row()

    def add_row(self):
        row = self.sourceRow()
        self._item_model.add_row(row)

    def add_row_below(self):
        row = self.sourceRow()
        self._item_model.add_row(row)

    def duplicate_row(self):
        row = self.sourceRow()
        self._item_model.duplicate_row(row)

    def move_row_up(self):
        row = self.sourceRow()
        select = self.currentIndex().row() - 1
        self._item_model.move_row_up(row)
        self._table_widget.selectRow(select)

    def move_row_down(self):
        row = self.sourceRow()
        select = self.currentIndex().row() + 1
        self._item_model.move_row_down(row)
        self._table_widget.selectRow(select)

    def remove_row(self):
        row = self.sourceRow()
        self._item_model.removeRows(row, 1, QModelIndex())
