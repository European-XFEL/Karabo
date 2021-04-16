from qtpy.QtCore import Qt, QModelIndex
from qtpy.QtWidgets import QAbstractItemView, QMenu
from traits.api import Bool, Dict, Undefined, WeakRef

from karabo.common.api import KARABO_SCHEMA_MIN_SIZE, KARABO_SCHEMA_MAX_SIZE
from karabogui.binding.api import get_editor_value, get_default_value
from karabogui.controllers.api import BaseBindingController

import karabogui.icons as icons

from .edit_delegates import get_table_delegate
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

    # Internal traits
    _bindings = Dict
    _readonly = Bool(True)
    _item_model = WeakRef(TableModel)

    # ---------------------------------------------------------------------
    # Abstract Methods

    def create_widget(self, parent):
        assert self.model is not Undefined

        widget = KaraboTableView(parent=parent)
        widget.setSelectionBehavior(QAbstractItemView.SelectItems
                                    | QAbstractItemView.SelectRows)
        widget.horizontalHeader().setStretchLastSection(True)
        return widget

    def set_read_only(self, ro):
        if ro:
            self._readonly = True
            self.widget.setEditTriggers(QAbstractItemView.NoEditTriggers)
            self.widget.setAlternatingRowColors(True)
            if self.hasCustomMenu:
                self.widget.setContextMenuPolicy(Qt.CustomContextMenu)
                self.widget.customContextMenuRequested.connect(
                    self._custom_menu)
                self.widget.setSelectionMode(QAbstractItemView.SingleSelection)
            else:
                self.widget.setSelectionMode(QAbstractItemView.NoSelection)
        else:
            self._readonly = False
            self.widget.setSelectionMode(QAbstractItemView.SingleSelection)
            flags = (QAbstractItemView.DoubleClicked
                     | QAbstractItemView.AnyKeyPressed
                     | QAbstractItemView.SelectedClicked)
            self.widget.setEditTriggers(flags)
            self.widget.setContextMenuPolicy(Qt.CustomContextMenu)
            # A reconfigurable table element always has a menu
            handler = (self._custom_menu if self.hasCustomMenu
                       else self._context_menu)
            self.widget.customContextMenuRequested.connect(handler)
            self.widget.setAcceptDrops(True)
        self.widget.setFocusPolicy(Qt.NoFocus if ro else Qt.ClickFocus)
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
        if self.widget:
            self.widget.setParent(None)
            self.widget = None

    # ---------------------------------------------------------------------
    # Subclass Methods

    def create_delegates(self):
        """Create all the table delegates in the table element"""
        bindings = self._bindings
        keys = bindings.keys()
        if self._readonly:
            # If we are readOnly, we erase all edit delegates
            for column, key in enumerate(keys):
                self.widget.setItemDelegateForColumn(column, None)
        else:
            # Create binding specific edit delegates
            for column, key in enumerate(keys):
                binding = bindings[key]
                delegate = get_table_delegate(binding, self.widget)
                self.widget.setItemDelegateForColumn(column, delegate)

    def custom_menu(self, pos):
        """Subclass method for own custom menu if ``hasCustomMenu`` is `True`

        :param: pos: The position of the context menu event
        """

    def getBindings(self):
        """Subclass access for table bindings of the controller"""
        return self._bindings

    def isReadOnly(self):
        """Subclass access for readOnly property of the table controller"""
        return self._readonly

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
        self._bindings = binding.bindings
        self._item_model = TableModel(binding, self._on_user_edit,
                                      parent=self.widget)
        self._item_model.set_readonly(self._readonly)
        self.widget.setModel(self._item_model)
        self.widget.set_bindings(binding.bindings)
        self.create_delegates()

    def _custom_menu(self, pos):
        """Private method to direct to the custom context menu"""
        selection_model = self.widget.selectionModel()
        if selection_model is None:
            return

        return self.custom_menu(pos)

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
        self.widget.selectRow(row - 1)

    def _move_row_down(self):
        row = self.currentIndex().row()
        self._item_model.move_row_down(row)
        self.widget.selectRow(row + 1)

    def _remove_row(self):
        index = self.currentIndex()
        self._item_model.removeRows(index.row(), 1, QModelIndex())

    def _context_menu(self, pos):
        """The custom context menu of a reconfigurable table element"""
        selection_model = self.widget.selectionModel()
        if selection_model is None:
            # Note: We did not yet receive a schema and thus have no table and
            # selection model!
            return

        index = selection_model.currentIndex()
        menu = QMenu(parent=self.widget)
        if index.isValid():
            set_default_action = menu.addAction('Set Cell Default')
            set_default_action.triggered.connect(self._set_index_default)
            menu.addSeparator()

            up_action = menu.addAction(icons.arrowFancyUp, 'Move Row Up')
            up_action.triggered.connect(self._move_row_up)
            down_action = menu.addAction(icons.arrowFancyDown, 'Move Row Down')
            down_action.triggered.connect(self._move_row_down)
            menu.addSeparator()

            add_action = menu.addAction(icons.add, 'Add Row below')
            add_action.triggered.connect(self._add_row)
            du_action = menu.addAction(icons.editCopy, 'Duplicate Row below')
            du_action.triggered.connect(self._duplicate_row)

            remove_action = menu.addAction(icons.delete, 'Delete Row')
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
            add_action = menu.addAction(icons.add, 'Add Row below')
            add_action.triggered.connect(self._add_row)

        menu.exec_(self.widget.viewport().mapToGlobal(pos))

    # ---------------------------------------------------------------------

    def currentIndex(self):
        """Convenience method to get the currentIndex of the selection"""
        return self.widget.selectionModel().currentIndex()
