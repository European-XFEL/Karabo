#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from qtpy.QtCore import QModelIndex, QSortFilterProxyModel, Qt
from qtpy.QtWidgets import (
    QAbstractItemView, QAction, QHBoxLayout, QHeaderView, QInputDialog,
    QLayout, QLineEdit, QMenu, QPushButton, QVBoxLayout, QWidget)
from traits.api import Bool, Dict, Instance, Type, Undefined, WeakRef

import karabogui.icons as icons
from karabo.common.api import KARABO_SCHEMA_MAX_SIZE, KARABO_SCHEMA_MIN_SIZE
from karabogui.binding.api import get_default_value, get_editor_value
from karabogui.controllers.api import BaseBindingController

from .delegates import get_display_delegate, get_table_delegate
from .model import TableModel
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
        widget.setFocusPolicy(Qt.NoFocus if ro else Qt.ClickFocus)
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

        row_count = self._item_model.rowCount()

        # Remove rows if necessesary
        if row_count > len(value):
            start = len(value) - 1
            count = row_count - len(value)
            self._item_model.removeRows(start, count, QModelIndex(),
                                        from_device=True)

        # Add rows if necessary
        elif row_count < len(value):
            start = row_count
            count = len(value) - row_count
            self._item_model.insertRows(start, count, QModelIndex(),
                                        from_device=True)
        for r, row in enumerate(value):
            for c, key in enumerate(row.getKeys()):
                index = self._item_model.index(r, c, QModelIndex())
                self._item_model.setData(index, row[key], Qt.DisplayRole,
                                         from_device=True)

    def destroy_widget(self):
        if self._item_model is not None:
            self._item_model.setParent(None)
            self._item_model = None
        if self.widget:
            self.widget.setParent(None)
            self.widget = None

    # ---------------------------------------------------------------------
    # Subclass Methods

    def create_delegates(self):
        """Create all the table delegates in the table element"""
        bindings = self._bindings
        keys = bindings.keys()
        get_delegate = (get_display_delegate if self._readonly
                        else get_table_delegate)
        for column, key in enumerate(keys):
            binding = bindings[key]
            delegate = get_delegate(self.proxy, binding, self._table_widget)
            self._table_widget.setItemDelegateForColumn(column, delegate)

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

    def getModelData(self, row, column):
        """Get data from the sourceModel with `row` and `column`

        :param row: row of the data
        :param column: column of the data

        Note: Function added with Karabo > 2.13.X
        """
        return self.sourceModel().get_model_data(row, column)

    def getInstanceId(self):
        """Retrieve the `instanceId` of the root proxy of this controller

        Note: Function added with Karabo > 2.13.X
        """
        return self.proxy.root_proxy.device_id

    def currentIndex(self):
        """Convenience method to get the currentIndex of the selection"""
        return self._table_widget.selectionModel().currentIndex()

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
    # Action Slots

    def _set_index_default(self):
        index = self.currentIndex()
        key = list(self._bindings.keys())[index.column()]
        binding = self._bindings[key]
        default_value = get_default_value(binding, force=True)
        self._item_model.setData(index, default_value, role=Qt.EditRole)

    def _add_row(self):
        row = self.currentIndex().row()
        self._item_model.insertRows(row + 1, 1, QModelIndex())

    def _duplicate_row(self):
        row = self.currentIndex().row()
        self._item_model.duplicate_row(row)

    def _move_row_up(self):
        row = self.currentIndex().row()
        self._item_model.move_row_up(row)
        self._table_widget.selectRow(row - 1)

    def _move_row_down(self):
        row = self.currentIndex().row()
        self._item_model.move_row_down(row)
        self._table_widget.selectRow(row + 1)

    def _remove_row(self):
        index = self.currentIndex()
        self._item_model.removeRows(index.row(), 1, QModelIndex())

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

        index = selection_model.currentIndex()
        menu = QMenu(parent=self._table_widget)
        if index.isValid():
            set_default_action = menu.addAction("Set Cell Default")
            set_default_action.triggered.connect(self._set_index_default)
            menu.addSeparator()

            up_action = menu.addAction(icons.arrowFancyUp, "Move Row Up")
            up_action.triggered.connect(self._move_row_up)
            down_action = menu.addAction(icons.arrowFancyDown, "Move Row Down")
            down_action.triggered.connect(self._move_row_down)
            menu.addSeparator()

            add_action = menu.addAction(icons.add, "Add Row below")
            add_action.triggered.connect(self._add_row)
            du_action = menu.addAction(icons.editCopy, "Duplicate Row below")
            du_action.triggered.connect(self._duplicate_row)

            remove_action = menu.addAction(icons.delete, "Delete Row")
            remove_action.triggered.connect(self._remove_row)

            # Check for min and max size of the table
            row_count = self._item_model.rowCount()
            attributes = self.proxy.binding.attributes
            min_size = attributes.get(KARABO_SCHEMA_MIN_SIZE)
            max_size = attributes.get(KARABO_SCHEMA_MAX_SIZE)
            add_row = True if max_size is None else row_count + 1 <= max_size
            rm_row = True if min_size is None else row_count - 1 >= min_size

            # Set actions enabled or disabled!
            add_action.setEnabled(add_row)
            du_action.setEnabled(add_row)
            remove_action.setEnabled(rm_row)
        else:
            add_action = menu.addAction(icons.add, "Add Row below")
            add_action.triggered.connect(self._add_row)

        menu.exec(self._table_widget.viewport().mapToGlobal(pos))

    # Private interface
    # ---------------------------------------------------------------------

    def _on_user_edit(self, data):
        """Callback method used by `self._item_model` when data changes"""
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
        self._table_widget.set_bindings(binding.bindings)
        self.create_delegates()

    def _custom_menu(self, pos):
        """Private method to direct to the custom context menu"""
        selection_model = self._table_widget.selectionModel()
        if selection_model is None:
            return

        return self.custom_menu(pos)


# --------------------------------------------------------------------------
# Filter Table Controller


class SortFilterModel(QSortFilterProxyModel):
    def __init__(self, parent=None):
        super().__init__(parent)

    def get_model_data(self, row, column):
        """Relay the request of model data to the source model"""
        index = self.index(row, column)
        source_index = self.mapToSource(index)
        return self.sourceModel().get_model_data(source_index.row(),
                                                 source_index.column())

    def get_header_key(self, section):
        """Relay the request of header data to the source model"""
        return self.sourceModel().get_header_key(section)


class BaseFilterTableController(BaseTableController):
    searchLabel = Instance(QLineEdit)

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

        self.searchLabel = QLineEdit(widget)
        self.searchLabel.setToolTip(
            f"Search column: {self.model.filterKeyColumn}")

        clear_button = QPushButton("Clear", parent=widget)
        clear_button.clicked.connect(self.searchLabel.clear)

        hor_layout.addWidget(self.searchLabel)
        hor_layout.addWidget(clear_button)

        # Complete widget layout and return widget
        widget_layout.addLayout(hor_layout)
        widget_layout.addWidget(table_widget)
        widget.setLayout(widget_layout)

        ac_column_filter = QAction("Set Filter Column", table_widget)
        ac_column_filter.triggered.connect(self._change_filter_column)
        widget.addAction(ac_column_filter)

        if self.model.sortingEnabled:
            table_widget.setSortingEnabled(True)
            table_widget.sortByColumn(0, Qt.AscendingOrder)

        return widget

    def createFilterModel(self, item_model):
        """Create the filter model for the table"""
        filter_model = SortFilterModel(self.tableWidget())
        filter_model.setSourceModel(item_model)
        key = self.model.filterKeyColumn
        filter_model.setFilterKeyColumn(key)
        filter_model.setFilterRole(Qt.DisplayRole)
        filter_model.setFilterCaseSensitivity(False)
        filter_model.setFilterFixedString("")
        self.searchLabel.textChanged.connect(filter_model.setFilterFixedString)

        return filter_model

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
            self.searchLabel.setToolTip(f"Search column: {column}")
