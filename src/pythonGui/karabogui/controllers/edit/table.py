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
from PyQt4.QtCore import pyqtSlot, Qt, QModelIndex, QPoint, QTimer
from PyQt4.QtGui import QAbstractItemView, QMenu, QStyledItemDelegate
from traits.api import Bool, Instance, Int, on_trait_change

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
    _column_hash = Instance(Hash)
    _role = Int(Qt.DisplayRole)
    _item_model = Instance(TableModel)
    _recent_context_trigger = Bool(False)

    def create_widget(self, parent):
        widget = KaraboTableView(parent=parent)
        widget.setSelectionBehavior(QAbstractItemView.SelectItems |
                                    QAbstractItemView.SelectRows |
                                    QAbstractItemView.SelectColumns)
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
            self.widget.setEditTriggers(QAbstractItemView.SelectedClicked)
            # add context menu to cells
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
            self._set_column_schema(schema)
        elif binding is not None:
            schema = binding.attributes.get(KARABO_SCHEMA_ROW_SCHEMA)
            if schema is not None:
                self._set_column_schema(schema)
                self.model.column_schema = SchemaHashType.toString(schema)

    def value_update(self, proxy):
        value = get_editor_value(proxy, [])
        if self._item_model.rowCount(None) > len(value):
            start = len(value) - 1
            count = self._item_model.rowCount(None) - len(value)
            self._item_model.removeRows(start, count, QModelIndex(),
                                        from_device_update=True)

        # add rows if necessary
        if self._item_model.rowCount(None) < len(value):
            start = self._item_model.rowCount(None)
            count = len(value) - self._item_model.rowCount(None)
            self._item_model.insertRows(start, count, QModelIndex(),
                                        from_device_update=True)

        for r, row in enumerate(value):
            for c, key in enumerate(row.getKeys()):
                idx = self._item_model.index(r, c, QModelIndex())
                self._item_model.setData(idx, row[key], self._role,
                                         from_device_update=True)

    @on_trait_change('model.column_schema')
    def _model_schema_update(self):
        if self.widget is None:
            return
        schema = SchemaHashType.fromstring(self.model.column_schema)
        self._set_column_schema(schema)

    def _on_user_edit(self, data):
        """Callback method used by `self._item_model` when data changes"""
        self.proxy.edit_value = data

    def _set_column_schema(self, schema):
        self._column_hash = schema.hash if schema is not None else Hash()
        self._item_model = TableModel(schema, self._on_user_edit)
        self._item_model.set_role(self._role)
        self.widget.setModel(self._item_model)
        self.widget.set_column_schema(schema)
        self._set_combo_boxes(self._role == Qt.DisplayRole)

    def _set_combo_boxes(self, ro):
        if self._column_hash is None:
            return

        c_hash = self._column_hash
        if ro:
            # remove any combo delegate
            for col, key in enumerate(c_hash.getKeys()):
                if c_hash.hasAttribute(key, 'options'):
                    delegate = QStyledItemDelegate()
                    self.widget.setItemDelegateForColumn(col, delegate)
        else:
            # add context menu to vertical header to add and remove rows
            v_header = self.widget.verticalHeader()
            v_header.setContextMenuPolicy(Qt.CustomContextMenu)
            v_header.customContextMenuRequested.connect(
                self._header_context_menu)

            # Create an item delegate for columns which have options
            for col, key in enumerate(c_hash.getKeys()):
                if c_hash.hasAttribute(key, 'options'):
                    delegate = ComboBoxDelegate(
                        c_hash.getAttribute(key, 'options'),
                        row=self._item_model.rowCount(None),
                        column=col, parent=self.widget
                    )
                    self.widget.setItemDelegateForColumn(col, delegate)

    @pyqtSlot(QPoint)
    def _context_menu(self, pos):
        """Context menu for single cells
        """
        # NOTE: This is a context menu for single cells, hence we bail out
        # if the entire row has been selected via the header
        if self.widget.selectionModel().selectedRows():
            return

        idx = None
        for i in self.widget.selectionModel().selection().indexes():
            idx = i
        menu = QMenu()
        if idx is None or not idx.isValid():
            add_action = menu.addAction('Add Row to end')
            action = menu.exec_(self.widget.viewport().mapToGlobal(pos))
            if action == add_action:
                start, count = self._item_model.rowCount(None), 1
                self._item_model.insertRows(start, count, QModelIndex())
            return

        # check if this cell can be set to a default value
        col = idx.column()
        set_default_action = None
        key = None

        if col >= 0 and col < len(self._column_hash):
            key = self._column_hash.getKeys()[col]

            if (self._column_hash.hasAttribute(key, 'defaultValue') and
                    self._role == Qt.EditRole):
                set_default_action = menu.addAction('Set to Default')

            # add a hint to the object type
            vtype = self._column_hash.getAttribute(key, 'valueType')
            type_dummy_action = menu.addAction(vtype)
            type_dummy_action.setEnabled(False)

        action = menu.exec_(self.widget.viewport().mapToGlobal(pos))
        if (action == set_default_action and key is not None and
                set_default_action is not None):
            defaultValue = self._column_hash.getAttribute(key, 'defaultValue')
            self._item_model.setData(idx, defaultValue, Qt.EditRole)

    @pyqtSlot(QPoint)
    def _header_context_menu(self, pos):
        if self._recent_context_trigger:
            return

        idx = None
        for i in self.widget.selectionModel().selection().indexes():
            idx = i

        menu = QMenu()
        if idx is not None:
            add_action = menu.addAction('Add Row below')
            dupe_action = menu.addAction('Duplicate Row below')
            remove_action = menu.addAction('Delete Row')
            action = menu.exec(self.widget.viewport().mapToGlobal(pos))
            if action == add_action:
                self._item_model.insertRows(idx.row() + 1, 1, QModelIndex())
            elif action == remove_action:
                self._item_model.removeRows(idx.row(), 1, QModelIndex())
            elif action == dupe_action:
                self._item_model.duplicate_row(idx.row())
        else:
            # try if we get are at a row nevertheless
            idx = self.widget.indexAt(pos)
            if idx.isValid():
                add_action = menu.addAction('Add Row below')
                dupe_action = menu.addAction('Duplicate Row below')
                action = menu.exec(self.widget.viewport().mapToGlobal(pos))
                if action == add_action:
                    start, count = idx.row() + 1, 1
                    self._item_model.insertRows(start, count, QModelIndex())
                elif action == dupe_action:
                    self._item_model.duplicate_row(idx.row())
            else:
                add_action = menu.addAction('Add Row to end')
                action = menu.exec(self.widget.viewport().mapToGlobal(pos))
                if action == add_action:
                    start, count = self._item_model.rowCount(None), 1
                    self._item_model.insertRows(start, count, QModelIndex())

        @pyqtSlot()
        def _clear_context_trigger():
            self._recent_context_trigger = False

        # avoid self triggering of the menu
        self._recent_context_trigger = True
        QTimer.singleShot(200, _clear_context_trigger)


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
