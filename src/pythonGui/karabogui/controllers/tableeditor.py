#############################################################################
# Author: <steffen.hauf@xfel.eu>
# Created on August 10, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import copy
import json

from PyQt4.QtCore import pyqtSlot, Qt, QAbstractTableModel, QModelIndex
from PyQt4.QtGui import QTableView, QComboBox, QItemDelegate

from karabo.common.api import KARABO_SCHEMA_VALUE_TYPE
from karabo.middlelayer import AccessMode, Hash
from karabogui.enums import NavigationItemTypes


def _value_type(schema, key):
    """Extract a single key's value type from a schema"""
    return schema.hash[key, KARABO_SCHEMA_VALUE_TYPE]


class TableModel(QAbstractTableModel):
    def __init__(self, column_schema, editing_finished, parent=None):
        super(QAbstractTableModel, self).__init__(parent)

        self._editing_finished = editing_finished
        self._column_schema = column_schema
        self._column_hash = (column_schema.hash if column_schema is not None
                             else Hash())
        self._role = Qt.EditRole
        self._data = []

    # ------------------------------------------------------------------------
    # Public interface

    def duplicate_row(self, pos):
        self.insertRows(pos + 1, 1, QModelIndex(), copy_row=self._data[pos])

    def set_role(self, role):
        self._role = role

    # ------------------------------------------------------------------------
    # QAbstractItemModel methods

    def rowCount(self, parent):
        return len(self._data)

    def columnCount(self, parent):
        return len(self._column_hash)

    def data(self, idx, role):
        if not idx.isValid():
            return None

        row, col = idx.row(), idx.column()
        if row < 0 or row >= len(self._data):
            return None

        if col < 0 or col >= len(self._column_hash):
            return None

        if role == Qt.CheckStateRole and self._role == Qt.EditRole:
            key = self._column_hash.getKeys()[col]
            value = self._data[row][key]
            vtype = _value_type(self._column_schema, key)
            if vtype == 'BOOL':
                return Qt.Checked if value else Qt.Unchecked

        if role == Qt.DisplayRole or role == Qt.EditRole:
            key = self._column_hash.getKeys()[col]
            value = self._data[row][key]
            vtype = _value_type(self._column_schema, key)
            if vtype.startswith('VECTOR'):
                # Guard against None values
                value = [] if value is None else value
                return ", ".join(str(v) for v in value)
            return str(value)

        return None

    def headerData(self, section, orientation, role):
        if role != Qt.DisplayRole:
            return None

        if orientation == Qt.Vertical:
            return str(section)

        if orientation == Qt.Horizontal:
            if section >= len(self._column_hash):
                return None

            key = self._column_hash.getKeys()[section]
            attrs = self._column_hash[key, ...]
            if 'displayedName' in attrs:
                return attrs['displayedName']
            return key

        return None

    def flags(self, idx):
        if not idx.isValid():
            return Qt.ItemIsEnabled

        base_flags = super(TableModel, self).flags(idx)
        key = self._column_hash.getKeys()[idx.column()]
        access = AccessMode(self._column_schema.hash[key, 'accessMode'])
        vtype = _value_type(self._column_schema, key)

        if vtype == 'BOOL' and self._role == Qt.EditRole:
            if access == AccessMode.READONLY:
                return (Qt.ItemIsUserCheckable | Qt.ItemIsSelectable
                        & ~Qt.ItemIsEnabled)

            return (Qt.ItemIsUserCheckable | Qt.ItemIsSelectable
                    | Qt.ItemIsEnabled)

        if access == AccessMode.READONLY:
            return base_flags & ~Qt.ItemIsEditable

        return base_flags | Qt.ItemIsEditable

    def setData(self, idx, value, role, from_device_update=False):
        if not idx.isValid():
            return False

        row, col = idx.row(), idx.column()
        if role == Qt.CheckStateRole:
            key = self._column_hash.getKeys()[col]
            vtype = _value_type(self._column_schema, key)
            if vtype == 'BOOL':
                value = True if value == Qt.Checked else False
                self._data[row][key] = value
                self.dataChanged.emit(idx, idx)
                if not from_device_update:
                    self._editing_finished(self._data)
                return True

        if role == Qt.EditRole or role == Qt.DisplayRole:
            if row < 0 or row >= len(self._data):
                return False

            if col < 0 or col >= len(self._column_hash):
                return False

            key = self._column_hash.getKeys()[col]
            vtype = _value_type(self._column_schema, key)
            # now display value
            if vtype.startswith('VECTOR') and not from_device_update:
                # this will be a list of individual chars we need to join
                value = "".join(value)
                value = [v.strip() for v in value.split(",")]
                # XXX: Formerly, the value was 'cast' here...
                # value = valueType.cast(value)

            self._data[row][key] = value

            self.dataChanged.emit(idx, idx)
            if role == Qt.EditRole and not from_device_update:
                self._editing_finished(self._data)
            return True

        return False

    def insertRows(self, pos, rows, idx, *,
                   copy_row=None, from_device_update=False):
        self.beginInsertRows(QModelIndex(), pos, pos + rows - 1)
        try:
            for r in range(rows):
                row_hash = copy.copy(copy_row)
                if row_hash is None:
                    row_hash = Hash()
                    for key in self._column_hash.getKeys():
                        attrs = self._column_hash[key, ...]
                        val = attrs.get('defaultValue', None)

                        # XXX: Formerly, the value was 'cast' here...
                        # val = valueType.cast(val)
                        row_hash[key] = val
                if pos + r < len(self._data):
                    self._data.insert(pos + r, row_hash)
                else:
                    self._data.append(row_hash)
        finally:
            self.endInsertRows()

        if not from_device_update:
            self._editing_finished(self._data)
        return True

    def removeRows(self, pos, rows, idx, *, from_device_update=False):
        # protect ourselves against invalid indices:
        end_pos = pos + rows - 1
        if pos < 0 or end_pos < 0:
            return False

        if end_pos > len(self._data) - 1:
            end_pos = len(self._data) - 1

        self.beginRemoveRows(QModelIndex(), pos, end_pos)
        try:
            for r in range(rows, 0, -1):
                self._data.pop(pos + r - 1)
        finally:
            self.endRemoveRows()

        if not from_device_update:
            self._editing_finished(self._data)
        return True


# from http://stackoverflow.com/questions/17615997/pyqt-how-to-set-qcombobox
# -in-a-table-view-using-qitemdelegate
class ComboBoxDelegate(QItemDelegate):
    def __init__(self, options, row=-1, column=-1, parent=None):
        super(ComboBoxDelegate, self).__init__(parent)
        self._options = options
        self._row_column = (row, column)
        parent.clicked.connect(self._cell_clicked)
        self._cur_cell_idx = None  # QPersistentModelIndex

    def createEditor(self, parent, option, index):
        combo = QComboBox(parent)
        combo.addItems([str(o) for o in self._options])
        combo.currentIndexChanged.connect(self._on_current_index_changed)
        return combo

    def setEditorData(self, editor, index):
        editor.blockSignals(True)
        selection = index.model().data(index, Qt.DisplayRole)
        editor.setCurrentIndex(self._options.index(selection))
        editor.blockSignals(False)

    def setModelData(self, editor, model, index):
        model.setData(index, self._options[editor.currentIndex()], Qt.EditRole)

    @pyqtSlot()
    def _on_current_index_changed(self):
        self.commitData.emit(self.sender())

    @pyqtSlot(object)
    def _cell_clicked(self, index):
        """Only enable editing for this delegate whenever user clicks on cell
        """
        # Only consider click events for this delegate in its column
        if (index.row(), index.column()) == self._row_column:
            if self._cur_cell_idx is not None:
                # Persistent model index and data namely QComboBox cleaned up
                self.parent().closePersistentEditor(self._cur_cell_idx)
            self._cur_cell_idx = index
            self.parent().openPersistentEditor(self._cur_cell_idx)
        else:
            if self._cur_cell_idx is not None:
                # Persistent model index and data namely QComboBox cleaned up
                self.parent().closePersistentEditor(self._cur_cell_idx)
            self._cur_cell_idx = None


class KaraboTableView(QTableView):
    def __init__(self, column_schema=None, parent=None):
        super(KaraboTableView, self).__init__(parent)
        self.set_column_schema(column_schema)

    def set_column_schema(self, column_schema):
        if column_schema is None:
            return

        self._column_schema = column_schema
        self._col_hash = column_schema.hash
        self._col_keys = self._col_hash.getKeys()
        self._first_string_column = None

        for c, key in enumerate(self._col_keys):
            vtype = _value_type(self._column_schema, key)
            if vtype == 'STRING':
                self._first_string_column = c
                break

    def dragEnterEvent(self, event):
        self._check_drag_event(event)

    def dragMoveEvent(self, event):
        self._check_drag_event(event)

    def dropEvent(self, event):
        acceptable, index, new_row, device_id = self._check_drag_event(event)

        if acceptable:
            model = self.model()
            if new_row:
                if self._first_string_column is not None:
                    model.insertRows(model.rowCount(None), 1, QModelIndex())
                    index = model.index(model.rowCount(None) - 1,
                                        self._first_string_column,
                                        QModelIndex())

                    # scroll to the end and pad with new whitespace to drop
                    # next item
                    self.scrollToBottom()

                else:
                    return
            model.setData(index, device_id, Qt.EditRole)

    def _check_drag_event(self, event):
        items_data = event.mimeData().data('treeItems').data()
        if len(items_data) == 0:
            event.ignore()
            return False, None, False, ''

        items = json.loads(items_data.decode())
        item = items[0]
        idx = self.indexAt(event.pos())
        nav_type = item.get('type')
        device_id = item.get('deviceId', '')
        from_project = device_id != ''
        usable = (nav_type == NavigationItemTypes.DEVICE or from_project)

        # drop in empty area is also okay but must trigger new_row
        if not idx.isValid() and usable:
            event.accept()
            return True, idx, True, device_id

        key = self._col_keys[idx.column()]
        vtype = _value_type(self._column_schema, key)
        if usable:
            event.accept()
            not_string = (vtype != 'STRING')
            return True, idx, not_string, device_id

        event.ignore()
        return False, None, False, device_id
