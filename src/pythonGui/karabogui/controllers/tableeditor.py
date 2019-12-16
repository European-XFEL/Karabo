#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on August 10, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import copy
import json

import numpy as np
from PyQt5.QtCore import pyqtSlot, Qt, QAbstractTableModel, QModelIndex
from PyQt5.QtGui import QBrush, QColor
from PyQt5.QtWidgets import QTableView, QComboBox, QItemDelegate

from karabo.common.api import (
    KARABO_SCHEMA_ACCESS_MODE, KARABO_SCHEMA_DEFAULT_VALUE,
    KARABO_SCHEMA_DISPLAY_TYPE, KARABO_SCHEMA_VALUE_TYPE,
    KARABO_SCHEMA_DISPLAY_TYPE_STATE, KARABO_SCHEMA_DISPLAYED_NAME)
from karabo.native import AccessMode, Hash
from karabogui.enums import NavigationItemTypes, ProjectItemTypes
from karabogui.indicators import get_state_color


def _get_value_type(schema, key):
    """Extract a single key's value type from a schema"""
    return schema.hash[key, KARABO_SCHEMA_VALUE_TYPE]


def _get_display_type(schema, key):
    """Extract the display type of a key from a schema"""
    return schema.hash[key, ...].get(KARABO_SCHEMA_DISPLAY_TYPE, '')


def _get_access_mode(schema, key):
    """Extract the access mode of a key from a schema"""
    access = schema.hash[key, ...].get(KARABO_SCHEMA_ACCESS_MODE)
    return AccessMode(access)


class TableModel(QAbstractTableModel):
    def __init__(self, row_schema, editing_finished, parent=None):
        super(QAbstractTableModel, self).__init__(parent)
        self._set_edit_value = editing_finished
        self._row_schema = row_schema
        self._row_hash = row_schema.hash
        self._role = Qt.EditRole
        self._data = []

    def rowCount(self, parent=None):
        """Reimplemented function of QAbstractTableModel"""
        return len(self._data)

    def columnCount(self, parent=None):
        """Reimplemented function of QAbstractTableModel"""
        return len(self._row_hash)

    def data(self, index, role):
        """Reimplemented function of QAbstractTableModel"""
        if not index.isValid():
            return None

        row, column = index.row(), index.column()
        if role == Qt.CheckStateRole and self._role == Qt.EditRole:
            # XXX: Again the Qt.EditRole check is to distinguish INITONLY
            # and READONLY access!
            key = self._row_hash.getKeys()[column]
            value = self._data[row][key]
            vtype = _get_value_type(self._row_schema, key)
            if vtype == 'BOOL':
                return Qt.Checked if value else Qt.Unchecked

        if role in (Qt.DisplayRole, Qt.EditRole):
            key = self._row_hash.getKeys()[column]
            value = self._data[row][key]
            vtype = _get_value_type(self._row_schema, key)
            if vtype.startswith('VECTOR'):
                # Guard against None values
                value = [] if value is None else value
                return ", ".join(str(v) for v in value)
            return str(value)

        elif role == Qt.BackgroundRole:
            key = self._row_hash.getKeys()[column]
            display_type = _get_display_type(self._row_schema, key)
            if display_type == KARABO_SCHEMA_DISPLAY_TYPE_STATE:
                value = self._data[row][key]
                color = get_state_color(value)
                if color is not None:
                    return QBrush(QColor(*color))

        return None

    def headerData(self, section, orientation, role):
        """Reimplemented function of QAbstractTableModel"""
        if role != Qt.DisplayRole:
            return None

        if orientation == Qt.Vertical:
            return str(section)

        if orientation == Qt.Horizontal:
            key = self._row_hash.getKeys()[section]
            attrs = self._row_hash[key, ...]
            return attrs.get(KARABO_SCHEMA_DISPLAYED_NAME, key)

        return None

    def flags(self, index):
        """Reimplemented function of QAbstractTableModel"""
        if not index.isValid():
            return Qt.NoItemFlags

        flags = Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable
        key = self._row_hash.getKeys()[index.column()]
        access = _get_access_mode(self._row_schema, key)
        vtype = _get_value_type(self._row_schema, key)

        if vtype == 'BOOL' and self._role == Qt.EditRole:
            flags &= ~Qt.ItemIsEditable
            if access is AccessMode.READONLY:
                # XXX: Remove the `enabled` flag due to INITONLY!
                flags &= ~Qt.ItemIsEnabled
            else:
                flags |= Qt.ItemIsUserCheckable
        elif access is AccessMode.READONLY:
            flags &= ~Qt.ItemIsEditable

        return flags

    def setData(self, index, value, role, from_device_update=False):
        """Reimplemented function of QAbstractTableModel"""
        if not index.isValid():
            return False

        row, col = index.row(), index.column()
        if role == Qt.CheckStateRole:
            key = self._row_hash.getKeys()[col]
            vtype = _get_value_type(self._row_schema, key)
            if vtype == 'BOOL':
                value = True if value == Qt.Checked else False
                self._data[row][key] = value
                self.dataChanged.emit(index, index)
                if not from_device_update:
                    self._set_edit_value(self._data)
                return True

            return False

        if role in (Qt.DisplayRole, Qt.EditRole):
            key = self._row_hash.getKeys()[col]
            vtype = _get_value_type(self._row_schema, key)
            # now display value
            if vtype.startswith('VECTOR') and not from_device_update:
                # this will be a list of individual chars we need to join
                value = "".join(value)
                value = [v.strip() for v in value.split(",")]
                # XXX: Formerly, the value was 'cast' here...
                # value = valueType.cast(value)

            self._data[row][key] = value

            self.dataChanged.emit(index, index)
            if role == Qt.EditRole and not from_device_update:
                self._set_edit_value(self._data)
            return True

        return False

    def insertRows(self, pos, rows, index, *,
                   copy_row=None, from_device_update=False):
        """Reimplemented function of QAbstractTableModel"""
        self.beginInsertRows(QModelIndex(), pos, pos + rows - 1)
        try:
            for row_nr in range(rows):
                row_hash = copy.copy(copy_row)
                if row_hash is None:
                    row_hash = Hash()
                    for key in self._row_hash.getKeys():
                        attrs = self._row_hash[key, ...]
                        val = attrs.get(KARABO_SCHEMA_DEFAULT_VALUE, None)

                        # XXX: Formerly, the value was 'cast' here...
                        # val = valueType.cast(val)
                        row_hash[key] = val
                if pos + row_nr < len(self._data):
                    self._data.insert(pos + row_nr, row_hash)
                else:
                    self._data.append(row_hash)
        finally:
            self.endInsertRows()

        if not from_device_update:
            self._set_edit_value(self._data)
        return True

    def removeRows(self, pos, rows, index, *, from_device_update=False):
        """Reimplemented function of QAbstractTableModel"""
        self.beginRemoveRows(QModelIndex(), pos, pos + rows - 1)
        try:
            for row_nr in range(rows, 0, -1):
                self._data.pop(pos + row_nr - 1)
        finally:
            self.endRemoveRows()

        if not from_device_update:
            self._set_edit_value(self._data)

        return True

    # ------------------------------------------------------------------------
    # Additional methods

    def duplicate_row(self, pos):
        self.insertRows(pos + 1, 1, QModelIndex(), copy_row=self._data[pos])

    def set_role(self, role=Qt.DisplayRole):
        """Set the role of the table element"""
        self._role = role


class ComboBoxDelegate(QItemDelegate):
    def __init__(self, options, parent=None):
        super(ComboBoxDelegate, self).__init__(parent)
        # XXX: We might have options from number values, they serialize as
        # ndarray! Once the value casting is back, the string cast has
        # to be removed!
        if isinstance(options, np.ndarray):
            options = options.astype(np.str).tolist()
        self._options = options

    def createEditor(self, parent, option, index):
        """Reimplemented function of QItemDelegate"""
        combo = QComboBox(parent)
        combo.addItems([str(o) for o in self._options])
        combo.currentIndexChanged.connect(self._on_current_index_changed)
        return combo

    def setEditorData(self, editor, index):
        """Reimplemented function of QItemDelegate"""
        editor.blockSignals(True)
        selection = index.model().data(index, Qt.DisplayRole)
        editor.setCurrentIndex(self._options.index(selection))
        editor.blockSignals(False)

    def setModelData(self, editor, model, index):
        """Reimplemented function of QItemDelegate"""
        model.setData(index, self._options[editor.currentIndex()], Qt.EditRole)

    @pyqtSlot()
    def _on_current_index_changed(self):
        """The current index of the combobox changed. Notify the model.

        This signal MUST be emitted when the editor widget has completed
        editing the data, and wants to write it back into the model.
        """
        self.commitData.emit(self.sender())


class KaraboTableView(QTableView):
    def __init__(self, row_schema=None, parent=None):
        super(KaraboTableView, self).__init__(parent)
        self.set_row_schema(row_schema)

    def set_row_schema(self, row_schema):
        if row_schema is None:
            return

        self._row_schema = row_schema
        self._row_hash = row_schema.hash
        self._row_keys = self._row_hash.getKeys()
        self._drag_column = None
        # XXX: Evaluate the eventual `drag-column`. If a string element is
        # found in the row schema, the first appearance is taken!
        for column_index, key in enumerate(self._row_keys):
            vtype = _get_value_type(self._row_schema, key)
            if vtype == 'STRING':
                self._drag_column = column_index
                break

    def dragEnterEvent(self, event):
        self._validate_drag_event(event)

    def dragMoveEvent(self, event):
        self._validate_drag_event(event)

    def dropEvent(self, event):
        is_valid, index, new_row, deviceId = self._validate_drag_event(event)

        if is_valid:
            model = self.model()
            if new_row:
                if self._drag_column is not None:
                    model.insertRows(model.rowCount(), 1, QModelIndex())
                    index = model.index(model.rowCount() - 1,
                                        self._drag_column,
                                        QModelIndex())
                    # scroll to the end and pad with new whitespace to drop
                    # next item
                    self.scrollToBottom()
                else:
                    return
            model.setData(index, deviceId, Qt.EditRole)

    def _validate_drag_event(self, event):
        items = event.mimeData().data('treeItems').data()
        if len(items) == 0:
            event.ignore()
            return False, None, False, 'None'

        item = json.loads(items.decode())[0]
        index = self.indexAt(event.pos())
        item_type = item.get('type')
        is_navigation_device = item_type == NavigationItemTypes.DEVICE
        is_project_device = item_type == ProjectItemTypes.DEVICE
        is_valid = is_navigation_device or is_project_device
        deviceId = item.get('deviceId', 'None')

        # Drop in empty area is also okay but must trigger new_row
        if not index.isValid() and is_valid:
            event.accept()
            return True, index, True, deviceId

        key = self._row_keys[index.column()]
        vtype = _get_value_type(self._row_schema, key)
        if is_valid:
            event.accept()
            not_string = (vtype != 'STRING')
            return True, index, not_string, deviceId

        event.ignore()
        return False, None, False, deviceId
