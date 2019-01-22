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
    def __init__(self, row_schema, editing_finished, parent=None):
        super(QAbstractTableModel, self).__init__(parent)

        self._editing_finished = editing_finished
        self._row_schema = row_schema
        self._row_hash = (row_schema.hash if row_schema is not None
                          else Hash())
        self._role = Qt.EditRole
        self._data = []

    # ------------------------------------------------------------------------
    # Public interface

    def duplicate_row(self, pos):
        self.insertRows(pos + 1, 1, QModelIndex(), copy_row=self._data[pos])

    def set_role(self, role):
        self._role = role

    def rowCount(self, parent=None):
        """Reimplemented function of QAbstractTableModel.
        """
        return len(self._data)

    def columnCount(self, parent=None):
        """Reimplemented function of QAbstractTableModel.
        """
        return len(self._row_hash)

    def data(self, index, role):
        """Reimplemented function of QAbstractTableModel.
        """
        if not index.isValid():
            return None

        row, col = index.row(), index.column()

        if role == Qt.CheckStateRole and self._role == Qt.EditRole:
            key = self._row_hash.getKeys()[col]
            value = self._data[row][key]
            vtype = _value_type(self._row_schema, key)
            if vtype == 'BOOL':
                return Qt.Checked if value else Qt.Unchecked

        if role == Qt.DisplayRole or role == Qt.EditRole:
            key = self._row_hash.getKeys()[col]
            value = self._data[row][key]
            vtype = _value_type(self._row_schema, key)
            if vtype.startswith('VECTOR'):
                # Guard against None values
                value = [] if value is None else value
                return ", ".join(str(v) for v in value)
            return str(value)

        return None

    def headerData(self, section, orientation, role):
        """Reimplemented function of QAbstractTableModel.
        """
        if role != Qt.DisplayRole:
            return None

        if orientation == Qt.Vertical:
            return str(section)

        if orientation == Qt.Horizontal:
            if section >= len(self._row_hash):
                return None

            key = self._row_hash.getKeys()[section]
            attrs = self._row_hash[key, ...]
            if 'displayedName' in attrs:
                return attrs['displayedName']
            return key

        return None

    def flags(self, index):
        """Reimplemented function of QAbstractTableModel.
        """
        if not index.isValid():
            return Qt.NoItemFlags

        flags = Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable
        key = self._row_hash.getKeys()[index.column()]
        access = AccessMode(self._row_schema.hash[key, 'accessMode'])
        vtype = _value_type(self._row_schema, key)

        if vtype == 'BOOL' and self._role == Qt.EditRole:
            if access == AccessMode.READONLY:
                flags &= ~Qt.ItemIsEditable
                flags &= ~Qt.ItemIsEnabled
            else:
                flags |= Qt.ItemIsUserCheckable
        elif access == AccessMode.READONLY:
            flags &= ~Qt.ItemIsEditable

        return flags

    def setData(self, index, value, role, from_device_update=False):
        """Reimplemented function of QAbstractTableModel.
        """
        if not index.isValid():
            return False

        row, col = index.row(), index.column()
        if role == Qt.CheckStateRole:
            key = self._row_hash.getKeys()[col]
            vtype = _value_type(self._row_schema, key)
            if vtype == 'BOOL':
                value = True if value == Qt.Checked else False
                self._data[row][key] = value
                self.dataChanged.emit(index, index)
                if not from_device_update:
                    self._editing_finished(self._data)
                return True

        if role == Qt.EditRole or role == Qt.DisplayRole:
            key = self._row_hash.getKeys()[col]
            vtype = _value_type(self._row_schema, key)
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
                self._editing_finished(self._data)
            return True

        return False

    def insertRows(self, pos, rows, index, *,
                   copy_row=None, from_device_update=False):
        self.layoutAboutToBeChanged.emit()
        self.beginInsertRows(QModelIndex(), pos, pos + rows)
        try:
            for r in range(rows):
                row_hash = copy.copy(copy_row)
                if row_hash is None:
                    row_hash = Hash()
                    for key in self._row_hash.getKeys():
                        attrs = self._row_hash[key, ...]
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

        self.layoutChanged.emit()
        if not from_device_update:
            self._editing_finished(self._data)
        return True

    def removeRows(self, pos, rows, index, *, from_device_update=False):
        # protect ourselves against invalid indices by declaring layout change
        self.layoutAboutToBeChanged.emit()
        end_pos = pos + rows

        self.beginRemoveRows(QModelIndex(), pos, end_pos)
        try:
            for r in range(rows, 0, -1):
                self._data.pop(pos + r - 1)
        finally:
            self.endRemoveRows()

        self.layoutChanged.emit()
        if not from_device_update:
            self._editing_finished(self._data)

        return True


class ComboBoxDelegate(QItemDelegate):
    def __init__(self, options, row=-1, column=-1, parent=None):
        super(ComboBoxDelegate, self).__init__(parent)
        self._options = options
        self._row_column = (row, column)
        parent.clicked.connect(self._cell_clicked)
        self._cur_cell_index = None  # QPersistentModelIndex

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
            if self._cur_cell_index is not None:
                # Persistent model index and data namely QComboBox cleaned up
                self.parent().closePersistentEditor(self._cur_cell_index)
            self._cur_cell_index = index
            self.parent().openPersistentEditor(self._cur_cell_index)
        else:
            if self._cur_cell_index is not None:
                # Persistent model index and data namely QComboBox cleaned up
                self.parent().closePersistentEditor(self._cur_cell_index)
            self._cur_cell_index = None


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
        self._first_string_column = None

        for c, key in enumerate(self._row_keys):
            vtype = _value_type(self._row_schema, key)
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
                    model.insertRows(model.rowCount(), 1, QModelIndex())
                    index = model.index(model.rowCount() - 1,
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
        index = self.indexAt(event.pos())
        nav_type = item.get('type')
        device_id = item.get('deviceId', '')
        from_project = device_id != ''
        usable = (nav_type == NavigationItemTypes.DEVICE or from_project)

        # drop in empty area is also okay but must trigger new_row
        if not index.isValid() and usable:
            event.accept()
            return True, index, True, device_id

        key = self._row_keys[index.column()]
        vtype = _value_type(self._row_schema, key)
        if usable:
            event.accept()
            not_string = (vtype != 'STRING')
            return True, index, not_string, device_id

        event.ignore()
        return False, None, False, device_id
