#############################################################################
# Author: <steffen.hauf@xfel.eu> & <dennis.goeries@xfel.eu>
# Created on August 10, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt5.QtCore import Qt, QModelIndex
from PyQt5.QtWidgets import QAbstractItemView, QMenu
from traits.api import Bool, Dict, Instance, WeakRef

from karabo.common.api import (
    KARABO_SCHEMA_DEFAULT_VALUE, KARABO_SCHEMA_ROW_SCHEMA)
from karabo.common.scenemodel.api import TableElementModel
from karabogui.binding.api import VectorHashBinding, get_editor_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.controllers.table.api import (
    get_table_delegate, KaraboTableView, TableModel)
import karabogui.icons as icons


class _BaseTableElement(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(TableElementModel, args=())
    # Internal traits
    _bindings = Dict
    _is_readonly = Bool
    _item_model = WeakRef(TableModel)

    def create_widget(self, parent):
        widget = KaraboTableView(parent=parent)
        widget.setSelectionBehavior(QAbstractItemView.SelectItems
                                    | QAbstractItemView.SelectRows)
        widget.horizontalHeader().setStretchLastSection(True)
        return widget

    def set_read_only(self, ro):
        if ro:
            self._is_readonly = True
            self.widget.setEditTriggers(QAbstractItemView.NoEditTriggers)
            self.widget.setAlternatingRowColors(True)
            self.widget.setSelectionMode(QAbstractItemView.NoSelection)
        else:
            self._is_readonly = False
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
            self._item_model.set_readonly(self._is_readonly)
            # We must set the delegates firstly on readOnly information!
            self._create_delegates(self._is_readonly)

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
                self._item_model.setData(index, row[key], self._is_readonly,
                                         is_device_update=True)

    def _on_user_edit(self, data):
        """Callback method used by `self._item_model` when data changes"""
        self.proxy.edit_value = data

    def destroy_widget(self):
        if self._item_model is not None:
            self._item_model.setParent(None)
        if self.widget:
            self.widget.setParent(None)
            self.widget = None

    def _set_bindings(self, binding):
        """Configure the column schema hashes and keys

        The schema must not be `None` and is protected when calling this func.
        """
        if self._item_model is not None:
            self._item_model.setParent(None)
        self._bindings = binding.bindings
        self._item_model = TableModel(binding, self._on_user_edit,
                                      parent=self.widget)
        self._item_model.set_readonly(self._is_readonly)
        self.widget.setModel(self._item_model)
        self.widget.set_bindings(binding.bindings)
        self._create_delegates(self._is_readonly)

    def _create_delegates(self, ro):
        """Create all the table delegates in the table element"""
        bindings = self._bindings
        keys = bindings.keys()
        if ro:
            # If we are readOnly, we erase all edit delegates
            for column, key in enumerate(keys):
                self.widget.setItemDelegateForColumn(column, None)
        else:
            # Create binding specific edit delegates
            for column, key in enumerate(keys):
                binding = bindings[key]
                delegate = get_table_delegate(binding, self.widget)
                self.widget.setItemDelegateForColumn(column, delegate)

    # ---------------------------------------------------------------------
    # Action Slots

    def _set_index_default(self):
        index = self.currentIndex()
        key = list(self._bindings.keys())[index.column()]
        attributes = self._bindings[key].attributes
        default_value = attributes[KARABO_SCHEMA_DEFAULT_VALUE]
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

    # ---------------------------------------------------------------------

    def currentIndex(self):
        """Convenience method to get the currentIndex of the selection"""
        return self.widget.selectionModel().currentIndex()

    def _context_menu(self, pos):
        selection_model = self.widget.selectionModel()
        if selection_model is None:
            # XXX: We did not yet receive a schema and thus have no table and
            # selection model!
            return
        index = selection_model.currentIndex()

        menu = QMenu(parent=self.widget)
        if index.isValid():
            column = index.column()
            key = list(self._bindings.keys())[column]
            if (not self._is_readonly and self._bindings[key].attributes.get(
                    KARABO_SCHEMA_DEFAULT_VALUE)):
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
