#############################################################################
# Author: <steffen.hauf@xfel.eu> & <dennis.goeries@xfel.eu>
# Created on August 10, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import copy
import json

from PyQt5.QtCore import Qt, QAbstractTableModel, QModelIndex
from PyQt5.QtGui import QBrush, QColor
from PyQt5.QtWidgets import QTableView

from karabogui.binding.api import BoolBinding, StringBinding, VectorBinding
from karabo.common.api import KARABO_SCHEMA_DEFAULT_VALUE
from karabo.native import AccessMode, Hash
from karabogui.enums import NavigationItemTypes, ProjectItemTypes
from karabogui.indicators import get_state_color

from .utils import is_state_display_type


class TableModel(QAbstractTableModel):
    def __init__(self, binding, set_edit_value, parent=None):
        super(QAbstractTableModel, self).__init__(parent)
        self._set_edit_value = set_edit_value
        self._role = Qt.EditRole
        self._data = []
        self._bindings = binding.bindings
        self._header = list(binding.bindings.keys())

    def rowCount(self, parent=None):
        """Reimplemented function of QAbstractTableModel"""
        return len(self._data)

    def columnCount(self, parent=None):
        """Reimplemented function of QAbstractTableModel"""
        return len(self._header)

    def data(self, index, role):
        """Reimplemented function of QAbstractTableModel"""
        if not index.isValid():
            return None

        row, column = index.row(), index.column()
        if role == Qt.CheckStateRole and self._role == Qt.EditRole:
            # XXX: Again the Qt.EditRole check is to distinguish INITONLY
            # and READONLY access!
            key = self._header[column]
            value = self._data[row][key]
            binding = self._bindings[key]
            if isinstance(binding, BoolBinding):
                return Qt.Checked if value else Qt.Unchecked

        if role in (Qt.DisplayRole, Qt.EditRole):
            key = self._header[column]
            value = self._data[row][key]
            binding = self._bindings[key]
            if isinstance(binding, VectorBinding):
                # Guard against None values
                value = [] if value is None else value
                return ", ".join(str(v) for v in value)
            return str(value)

        elif role == Qt.BackgroundRole:
            key = self._header[column]
            if is_state_display_type(self._bindings[key]):
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
            key = self._header[section]
            return self._bindings[key].displayed_name

        return None

    def flags(self, index):
        """Reimplemented function of QAbstractTableModel"""
        if not index.isValid():
            return Qt.NoItemFlags

        flags = Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable
        key = self._header[index.column()]
        # Get an enum for the AccessMode
        binding = self._bindings[key]
        access_mode = binding.access_mode
        if isinstance(binding, BoolBinding) and self._role == Qt.EditRole:
            flags &= ~Qt.ItemIsEditable
            if access_mode is AccessMode.READONLY:
                # XXX: Remove the `enabled` flag due to INITONLY!
                flags &= ~Qt.ItemIsEnabled
            else:
                flags |= Qt.ItemIsUserCheckable
        elif access_mode is AccessMode.READONLY:
            flags &= ~Qt.ItemIsEditable

        return flags

    def setData(self, index, value, role, is_device_update=False):
        """Reimplemented function of QAbstractTableModel"""
        if not index.isValid():
            return False

        row, column = index.row(), index.column()
        if role == Qt.CheckStateRole:
            key = self._header[column]
            binding = self._bindings[key]
            if isinstance(binding, BoolBinding):
                value = True if value == Qt.Checked else False
                self._data[row][key] = value
                self.dataChanged.emit(index, index)
                if not is_device_update:
                    self._set_edit_value(self._data)
                return True

            return False

        if role in (Qt.DisplayRole, Qt.EditRole):
            key = self._header[column]
            binding = self._bindings[key]
            # now display value
            if isinstance(binding, VectorBinding) and not is_device_update:
                # this will be a list of individual chars we need to join
                value = "".join(value)
                value = [v.strip() for v in value.split(",")]
                # XXX: Formerly, the value was 'cast' here...
                # value = valueType.cast(value)

            self._data[row][key] = value
            self.dataChanged.emit(index, index)

            if role == Qt.EditRole and not is_device_update:
                self._set_edit_value(self._data)
            return True

        return False

    def insertRows(self, pos, rows, index, *,
                   copy_row=None, is_device_update=False):
        """Reimplemented function of QAbstractTableModel"""
        self.beginInsertRows(QModelIndex(), pos, pos + rows - 1)
        try:
            for row_nr in range(rows):
                column_hash = copy.copy(copy_row)
                if column_hash is None:
                    column_hash = Hash()
                    for key in self._header:
                        attrs = self._bindings[key].attributes
                        value = attrs.get(KARABO_SCHEMA_DEFAULT_VALUE, None)

                        # XXX: Formerly, the value was 'cast' here...
                        # val = valueType.cast(val)
                        column_hash[key] = value
                if pos + row_nr < len(self._data):
                    self._data.insert(pos + row_nr, column_hash)
                else:
                    self._data.append(column_hash)
        finally:
            self.endInsertRows()

        if not is_device_update:
            self._set_edit_value(self._data)
        return True

    def removeRows(self, pos, rows, index, *, is_device_update=False):
        """Reimplemented function of QAbstractTableModel"""
        self.beginRemoveRows(QModelIndex(), pos, pos + rows - 1)
        try:
            for row_nr in range(rows, 0, -1):
                self._data.pop(pos + row_nr - 1)
        finally:
            self.endRemoveRows()

        if not is_device_update:
            self._set_edit_value(self._data)

        return True

    def clear_model(self):
        """Clear the model and remove all dat"""
        self.beginResetModel()
        try:
            self._data = []
        finally:
            self.endResetModel()

    def move_row_up(self, row):
        """In a simple two step process move a row element up"""
        if row > 0:
            copy_row = self._data[row]
            self.removeRows(row, 1, QModelIndex())
            self.insertRows(row - 1, 1, QModelIndex(), copy_row=copy_row)

    def move_row_down(self, row):
        """In a simple two step process move a row element down"""
        if row < self.rowCount() - 1:
            copy_row = self._data[row]
            self.removeRows(row, 1, QModelIndex())
            self.insertRows(row + 1, 1, QModelIndex(), copy_row=copy_row)

    # ------------------------------------------------------------------------
    # Additional methods

    def duplicate_row(self, pos):
        self.insertRows(pos + 1, 1, QModelIndex(), copy_row=self._data[pos])

    def set_role(self, role=Qt.DisplayRole):
        """Set the role of the table element"""
        self._role = role


class KaraboTableView(QTableView):
    def __init__(self, parent=None):
        super(KaraboTableView, self).__init__(parent)
        self._header = None
        self._bindings = None
        self._drag_column = None

    def set_bindings(self, bindings):
        self._bindings = bindings
        self._header = list(bindings.keys())
        # Note: Evaluate the eventual `drag-column`. If a string element is
        # found in the row schema, the first appearance is taken!
        for column_index, key in enumerate(self._header):
            binding = bindings[key]
            if isinstance(binding, StringBinding):
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
        if self._header is None:
            # Header is present after the schema has arrived.
            return False, None, False, 'None'

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

        key = self._header[index.column()]
        binding = self._bindings[key]
        if is_valid:
            event.accept()
            not_string = not isinstance(binding, StringBinding)
            return True, index, not_string, deviceId

        event.ignore()
        return False, None, False, deviceId