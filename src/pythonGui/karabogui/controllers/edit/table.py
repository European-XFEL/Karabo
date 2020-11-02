#############################################################################
# Author: <steffen.hauf@xfel.eu> & <dennis.goeries@xfel.eu>
# Created on August 10, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt5.QtCore import Qt, QModelIndex
from PyQt5.QtWidgets import QAbstractItemView, QMenu, QStyledItemDelegate
from traits.api import Instance, Int

from karabo.common.api import (
    KARABO_SCHEMA_DEFAULT_VALUE, KARABO_SCHEMA_ROW_SCHEMA,
    KARABO_SCHEMA_OPTIONS)
from karabo.common.scenemodel.api import TableElementModel
from karabo.native import Hash
from karabogui.binding.api import (
    VectorHashBinding, get_editor_value)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.controllers.table_editor import (
    ComboBoxDelegate, KaraboTableView, TableModel)
import karabogui.icons as icons


def _get_options(schema_hash, key):
    """Extract a single key's options from a hash

    If options are not specified, `None` is returned!
    """
    return schema_hash[key, ...].get(KARABO_SCHEMA_OPTIONS, None)


class _BaseTableElement(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(TableElementModel, args=())
    # Internal traits
    _column_hash = Instance(Hash)
    _role = Int(Qt.DisplayRole)
    _item_model = Instance(TableModel)

    def create_widget(self, parent):
        widget = KaraboTableView(parent=parent)
        widget.setSelectionBehavior(QAbstractItemView.SelectItems
                                    | QAbstractItemView.SelectRows)
        widget.horizontalHeader().setStretchLastSection(True)
        return widget

    def set_read_only(self, ro):
        if ro:
            self._role = Qt.DisplayRole
            self.widget.setEditTriggers(QAbstractItemView.NoEditTriggers)
            self.widget.setAlternatingRowColors(True)
            self.widget.setSelectionMode(QAbstractItemView.NoSelection)
        else:
            self._role = Qt.EditRole
            self.widget.setSelectionMode(QAbstractItemView.SingleSelection)
            flags = (QAbstractItemView.DoubleClicked
                     | QAbstractItemView.AnyKeyPressed
                     | QAbstractItemView.SelectedClicked)
            self.widget.setEditTriggers(flags)
            self.widget.setContextMenuPolicy(Qt.CustomContextMenu)
            self.widget.customContextMenuRequested.connect(self._context_menu)
            self.widget.setAcceptDrops(True)
        self.widget.setFocusPolicy(Qt.NoFocus if ro else Qt.ClickFocus)
        if self._item_model is not None:
            self._item_model.set_role(self._role)

    def binding_update(self, proxy):
        binding = proxy.binding
        if binding is not None:
            schema = binding.attributes.get(KARABO_SCHEMA_ROW_SCHEMA)
            if schema is not None:
                self._set_column_hash(schema)

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
                                        is_device_update=True)

        # Add rows if necessary
        elif row_count < len(value):
            start = row_count
            count = len(value) - row_count
            self._item_model.insertRows(start, count, QModelIndex(),
                                        is_device_update=True)
        for r, row in enumerate(value):
            for c, key in enumerate(row.getKeys()):
                index = self._item_model.index(r, c, QModelIndex())
                self._item_model.setData(index, row[key], self._role,
                                         is_device_update=True)

    def _on_user_edit(self, data):
        """Callback method used by `self._item_model` when data changes"""
        self.proxy.edit_value = data

    def _set_column_hash(self, schema):
        """Configure the column schema hashes and keys

        The schema must not be `None` and is protected when calling this func.
        """
        if self._item_model is not None:
            self._item_model.setParent(None)
            self._item_model = None

        self._column_hash = schema.hash
        self._item_model = TableModel(self._column_hash, self._on_user_edit,
                                      parent=self.widget)
        self._item_model.set_role(self._role)

        self.widget.setModel(self._item_model)
        self.widget.set_column_hash(self._column_hash)
        self._create_delegates(self._role == Qt.DisplayRole)

    def _create_delegates(self, ro):
        """Create all the combox delegates in the table element"""
        c_hash = self._column_hash
        c_keys = c_hash.getKeys()
        if ro:
            # If we are readOnly, we have to remove any combo delegate
            for column, key in enumerate(c_keys):
                options = _get_options(c_hash, key)
                if options is not None:
                    delegate = QStyledItemDelegate(parent=self.widget)
                    self.widget.setItemDelegateForColumn(column, delegate)
        else:
            # Create item delegates for columns which have options
            for column, key in enumerate(c_keys):
                options = _get_options(c_hash, key)
                if options is not None:
                    delegate = ComboBoxDelegate(options, parent=self.widget)
                    self.widget.setItemDelegateForColumn(column, delegate)

    # ---------------------------------------------------------------------
    # Actions

    def _row_to_end_action(self):
        start, count = self._item_model.rowCount(), 1
        self._item_model.insertRows(start, count, QModelIndex())

    def _set_index_default(self, index, key):
        default_value = self._column_hash[key, KARABO_SCHEMA_DEFAULT_VALUE]
        self._item_model.setData(index, default_value, role=Qt.EditRole)

    def _add_row(self, index):
        self._item_model.insertRows(index.row() + 1, 1, QModelIndex())

    def _duplicate_row(self, index):
        self._item_model.duplicate_row(index.row())

    def _move_row_up(self, index):
        row = index.row()
        self._item_model.move_row_up(row)
        self.widget.selectRow(row - 1)

    def _move_row_down(self, index):
        row = index.row()
        self._item_model.move_row_down(row)
        self.widget.selectRow(row + 1)

    def _remove_row(self, index):
        self._item_model.removeRows(index.row(), 1, QModelIndex())

    def _context_menu(self, pos):
        selection_model = self.widget.selectionModel()
        if selection_model is None:
            # XXX: We did not yet receive a schema and thus have no table and
            # selection model!
            return
        index = selection_model.currentIndex()

        menu = QMenu()
        if index is not None:
            column = index.column()
            key = self._column_hash.getKeys()[column]
            if (self._role == Qt.EditRole
                    and self._column_hash.hasAttribute(
                        key, KARABO_SCHEMA_DEFAULT_VALUE)):
                set_default_action = menu.addAction('Set Cell Default')
                set_default_action.triggered.connect(
                    partial(self._set_index_default, index=index, key=key))
                menu.addSeparator()

            up_action = menu.addAction(icons.arrowFancyUp, 'Move Row Up')
            up_action.triggered.connect(partial(self._move_row_up, index))
            down_action = menu.addAction(icons.arrowFancyDown, 'Move Row Down')
            down_action.triggered.connect(partial(self._move_row_down, index))
            menu.addSeparator()
            add_action = menu.addAction(icons.add, 'Add Row below')
            add_action.triggered.connect(partial(self._add_row, index))
            du_action = menu.addAction(icons.editCopy, 'Duplicate Row below')
            du_action.triggered.connect(partial(self._duplicate_row, index))
            remove_action = menu.addAction(icons.delete, 'Delete Row')
            remove_action.triggered.connect(partial(self._remove_row, index))

        menu.exec_(self.widget.viewport().mapToGlobal(pos))


def _is_compatible(binding):
    return KARABO_SCHEMA_ROW_SCHEMA in binding.attributes


@register_binding_controller(ui_name='Table Element',
                             klassname='EditableTableElement', can_edit=True,
                             binding_type=VectorHashBinding, priority=100,
                             is_compatible=_is_compatible)
class EditableTableElement(_BaseTableElement):
    """The editable version of the table element"""


@register_binding_controller(ui_name='Display Table Element',
                             klassname='DisplayTableElement', priority=90,
                             binding_type=VectorHashBinding,
                             is_compatible=_is_compatible)
class DisplayTableElement(_BaseTableElement):
    """The display version of the table element"""
