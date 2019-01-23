#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on August 10, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
"""
This module contains a class which represents a widget for tables and
is created as a composition of EditableWidget and DisplayWidget. Rendering in
read-only mode is controlled via the set readOnly method.

The element is applicable for VECTOR_HASH data types, which have a 'rowSchema'
attribute. The rowSchema is a Hash (schema.parameterHash to be precise), which
defines the column layout, i.e. column count, column data types and column
headers.

In case the rowSchema contains a 'displayedName' field this is used as the
column header, otherwise the field's key is used.

For string fields with options supplied the cell is rendered as a drop down
menu.
Boolean fields are rendered as check boxes.

Additional manipulation functionality includes, adding, deleting and
duplicating rows (the latter require a cell or row to be selected).

A right-click will display the cells data type both in Display and Edit mode.

The Table widget supports drag and drop of deviceId's from the navigation and
project panel. Dropping on a string cell will replace the string with the
deviceId.
Dropping on a non-string cell or on an empty region will add a row in which the
first string-type column encountered is pre-filled with the deviceID.
"""
from functools import partial

from PyQt4.QtCore import pyqtSlot, Qt, QModelIndex, QPoint
from PyQt4.QtGui import QAbstractItemView, QMenu, QStyledItemDelegate
from traits.api import Instance, Int

from karabo.common.api import KARABO_SCHEMA_ROW_SCHEMA
from karabo.common.scenemodel.api import TableElementModel
from karabo.middlelayer import Hash, SchemaHashType
from karabogui.binding.api import (
    VectorHashBinding, get_editor_value)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.controllers.tableeditor import (
    ComboBoxDelegate, KaraboTableView, TableModel)


class _BaseTableElement(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(TableElementModel, args=())
    # Internal traits
    _row_hash = Instance(Hash)
    _role = Int(Qt.DisplayRole)
    _item_model = Instance(TableModel)

    def create_widget(self, parent):
        widget = KaraboTableView(parent=parent)
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
            flags = (QAbstractItemView.DoubleClicked
                     | QAbstractItemView.AnyKeyPressed)
            self.widget.setEditTriggers(flags)
            self.widget.setContextMenuPolicy(Qt.CustomContextMenu)
            self.widget.customContextMenuRequested.connect(self._context_menu)
            self.widget.setAcceptDrops(True)

        self.widget.setFocusPolicy(Qt.NoFocus if ro else Qt.ClickFocus)
        if self._item_model is not None:
            self._item_model.set_role(self._role)
        self._set_combo_boxes(ro)

    def binding_update(self, proxy):
        binding = proxy.binding
        if binding is None and self.model.column_schema != '':
            # No binding yet, but the scene model has us covered
            schema = SchemaHashType.fromstring(self.model.column_schema)
            self._set_row_schema(schema)
        elif binding is not None:
            schema = binding.attributes.get(KARABO_SCHEMA_ROW_SCHEMA)
            if schema is not None:
                self._set_row_schema(schema)
                self.model.column_schema = SchemaHashType.toString(schema)

    def value_update(self, proxy):
        value = get_editor_value(proxy, [])
        if self._item_model.rowCount() > len(value):
            start = len(value) - 1
            count = self._item_model.rowCount() - len(value)
            self._item_model.removeRows(start, count, QModelIndex(),
                                        from_device_update=True)

        # add rows if necessary
        if self._item_model.rowCount() < len(value):
            start = self._item_model.rowCount()
            count = len(value) - self._item_model.rowCount()
            self._item_model.insertRows(start, count, QModelIndex(),
                                        from_device_update=True)
        for r, row in enumerate(value):
            for c, key in enumerate(row.getKeys()):
                index = self._item_model.index(r, c, QModelIndex())
                self._item_model.setData(index, row[key], self._role,
                                         from_device_update=True)

    def _on_user_edit(self, data):
        """Callback method used by `self._item_model` when data changes"""
        self.proxy.edit_value = data

    def _set_row_schema(self, schema):
        self._row_hash = schema.hash if schema is not None else Hash()
        self._item_model = TableModel(schema, self._on_user_edit)
        self._item_model.set_role(self._role)
        self.widget.setModel(self._item_model)
        self.widget.set_row_schema(schema)
        self._set_combo_boxes(self._role == Qt.DisplayRole)

    def _set_combo_boxes(self, ro):
        if self._row_hash is None:
            return

        c_hash = self._row_hash
        if ro:
            # remove any combo delegate
            for column, key in enumerate(c_hash.getKeys()):
                if c_hash.hasAttribute(key, 'options'):
                    delegate = QStyledItemDelegate(parent=self.widget)
                    self.widget.setItemDelegateForColumn(column, delegate)
        else:
            # Create an item delegate for columns which have options
            for column, key in enumerate(c_hash.getKeys()):
                if c_hash.hasAttribute(key, 'options'):
                    delegate = ComboBoxDelegate(
                        c_hash.getAttribute(key, 'options'),
                        row=self._item_model.rowCount(),
                        column=column, parent=self.widget)
                    self.widget.setItemDelegateForColumn(column, delegate)

# ---------------------------------------------------------------------
# Actions

    @pyqtSlot()
    def _row_to_end_action(self):
        start, count = self._item_model.rowCount(), 1
        self._item_model.insertRows(start, count, QModelIndex())

    @pyqtSlot(QModelIndex, str)
    def _set_row_default(self, index, key):
        default_value = self._row_hash.getAttribute(key, 'defaultValue')
        self._item_model.setData(index, default_value, role=Qt.EditRole)

    @pyqtSlot(QModelIndex)
    def _add_row(self, index):
        self._item_model.insertRows(index.row() + 1, 1, QModelIndex())

    @pyqtSlot(QModelIndex)
    def _dupe_row(self, index):
        self._item_model.duplicate_row(index.row())

    @pyqtSlot(QModelIndex)
    def _remove_row(self, index):
        self._item_model.removeRows(index.row(), 1, QModelIndex())

    @pyqtSlot(QPoint)
    def _context_menu(self, event_pos):
        pos = event_pos
        selection_model = self.widget.selectionModel()
        selection = selection_model.selection()
        indexes = selection.indexes()
        index = indexes[-1] if len(indexes) else None

        menu = QMenu()
        if index is not None:
            # NOTE: We have a selection and check first if we can set this
            # cell to a default value
            column = index.column()
            if column >= 0 and column < len(self._row_hash):
                key = self._row_hash.getKeys()[column]
                if (self._role == Qt.EditRole
                        and self._row_hash.hasAttribute(key, 'defaultValue')):
                    set_default_action = menu.addAction('Set Cell Default')
                    set_default_action.triggered.connect(
                        partial(self._set_row_default, index=index, key=key))
                    menu.addSeparator()

            add_action = menu.addAction('Add Row below')
            add_action.triggered.connect(partial(self._add_row, index))
            dupe_action = menu.addAction('Duplicate Row below')
            dupe_action.triggered.connect(partial(self._dupe_row, index))
            remove_action = menu.addAction('Delete Row')
            remove_action.triggered.connect(partial(self._remove_row, index))
        else:
            # We have no selection and are most probably out of bounds!
            end_action = menu.addAction('Add Row to End')
            end_action.triggered.connect(self._row_to_end_action)
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
