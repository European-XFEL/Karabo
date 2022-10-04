#############################################################################
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import copy

from qtpy.QtCore import QAbstractTableModel, QModelIndex, Qt
from qtpy.QtGui import QBrush, QColor

from karabo.native import AccessMode, Hash
from karabogui.binding.api import BoolBinding, VectorBinding, get_default_value
from karabogui.indicators import get_state_color

from .utils import is_state_display_type, list2string, string2list


class TableModel(QAbstractTableModel):
    def __init__(self, binding, set_edit_value, parent=None):
        super().__init__(parent)
        self._set_edit_value = set_edit_value
        self._readonly = False
        self._data = []
        self._bindings = binding.bindings
        self._header = list(binding.bindings.keys())

    # Qt Methods
    # ----------------------------------------------------------------------

    def rowCount(self, parent=None):
        """Reimplemented function of QAbstractTableModel"""
        return len(self._data)

    def columnCount(self, parent=None):
        """Reimplemented function of QAbstractTableModel"""
        return len(self._header)

    def data(self, index, role=Qt.DisplayRole):
        """Reimplemented function of QAbstractTableModel"""
        if not index.isValid():
            return None

        row, column = index.row(), index.column()
        if role == Qt.CheckStateRole:
            key = self._header[column]
            value = self._data[row][key]
            binding = self._bindings[key]
            if isinstance(binding, BoolBinding):
                return Qt.Checked if value else Qt.Unchecked
        elif role in (Qt.DisplayRole, Qt.EditRole):
            key = self._header[column]
            value = self._data[row][key]
            binding = self._bindings[key]
            if isinstance(binding, VectorBinding):
                value = list2string(value)
            return str(value)
        elif role == Qt.BackgroundRole:
            key = self._header[column]
            if is_state_display_type(self._bindings[key]):
                value = self._data[row][key]
                try:
                    color = get_state_color(value)
                except ValueError:
                    return None
                return QBrush(QColor(*color))
        elif role == Qt.ToolTipRole:
            key = self._header[column]
            value = self._data[row][key]
            binding = self._bindings[key]
            if isinstance(binding, VectorBinding):
                value = list2string(value)
            return str(value)

        return None

    def _build_tooltip(self, attributes):
        """Build a tooltip according to the attributes"""
        selection = ["displayedName", "defaultValue", "valueType",
                     "description", "unitSymbol", "metricPrefixSymbol",
                     "minInc", "maxInc", "minExc", "maxExc", "options"]
        info = {}
        for akey in selection:
            avalue = attributes.get(akey)
            if avalue is not None:
                info.update({akey: str(avalue)})

        return ("<table>" +
                "".join("<tr><td><b>{}</b>:   </td><td>{}</td></tr>".
                        format(attr, str(value))
                        for attr, value in info.items())
                + "</table>")

    def headerData(self, section, orientation, role):
        """Reimplemented function of QAbstractTableModel"""
        if role == Qt.DisplayRole:
            if orientation == Qt.Vertical:
                return str(section)
            elif orientation == Qt.Horizontal:
                key = self._header[section]
                binding = self._bindings[key]
                units = binding.unit_label
                # Make sure to always show a header!
                displayed = binding.displayed_name or key

                return f"{displayed} [{units}]" if units else displayed

        elif role == Qt.ToolTipRole and orientation == Qt.Horizontal:
            key = self._header[section]
            attrs = self._bindings[key].attributes
            return self._build_tooltip(attrs)

        return None

    def flags(self, index):
        """Reimplemented function of QAbstractTableModel"""
        if not index.isValid():
            return Qt.NoItemFlags

        flags = (Qt.ItemIsEnabled | Qt.ItemIsSelectable |
                 Qt.ItemIsEditable | Qt.ItemNeverHasChildren)
        key = self._header[index.column()]
        # Get an enum for the AccessMode
        binding = self._bindings[key]
        access_mode = binding.access_mode
        if isinstance(binding, BoolBinding) and not self._readonly:
            flags &= ~Qt.ItemIsEditable
            if access_mode is AccessMode.RECONFIGURABLE:
                flags |= Qt.ItemIsUserCheckable
        elif access_mode is AccessMode.READONLY:
            flags &= ~Qt.ItemIsEditable

        return flags

    def setData(self, index, value, role=Qt.EditRole):
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
                self._set_edit_value(self._data)
                return True

            return False

        elif role in (Qt.DisplayRole, Qt.EditRole):
            key = self._header[column]
            binding = self._bindings[key]
            if isinstance(binding, VectorBinding):
                value = string2list(value)
            value = binding.validate_trait("value", value)
            self._data[row][key] = value
            self.dataChanged.emit(index, index)
            self._set_edit_value(self._data)
            return True

        return False

    def updateData(self, data):
        """External quick update of the table data"""
        roles = [Qt.BackgroundRole, Qt.CheckStateRole, Qt.DisplayRole,
                 Qt.ToolTipRole]
        rows = self.rowCount() - 1
        columns = self.columnCount() - 1
        for row, hsh in enumerate(data):
            self._data[row].update(hsh)

        first_index = self.index(0, 0)
        last_index = self.index(rows, columns)
        self.dataChanged.emit(first_index, last_index, roles)

    def insertRows(self, pos, rows, index, *,
                   copy_row=None, from_device=False):
        """Reimplemented function of QAbstractTableModel"""
        self.beginInsertRows(QModelIndex(), pos, pos + rows - 1)
        try:
            for row_nr in range(rows):
                column_hash = copy.copy(copy_row)
                if column_hash is None:
                    column_hash = Hash()
                    for key in self._header:
                        binding = self._bindings[key]
                        value = get_default_value(binding, force=True)
                        value = binding.validate_trait("value", value)
                        column_hash[key] = value
                if pos + row_nr < len(self._data):
                    self._data.insert(pos + row_nr, column_hash)
                else:
                    self._data.append(column_hash)
        finally:
            self.endInsertRows()

        if not from_device:
            self._set_edit_value(self._data)
        return True

    def removeRows(self, pos, rows, index, *, from_device=False):
        """Reimplemented function of QAbstractTableModel"""
        self.beginRemoveRows(QModelIndex(), pos, pos + rows - 1)
        try:
            for row_nr in range(rows, 0, -1):
                self._data.pop(pos + row_nr - 1)
        finally:
            self.endRemoveRows()
        if not from_device:
            self._set_edit_value(self._data)

        return True

    # Public Interface
    # -----------------------------------------------------------------------

    def index_ref(self, index):
        return index

    def get_model_data(self, row, column):
        """Retrieve the validated model data for `row` and `column`"""
        key = self._header[column]
        binding = self._bindings[key]

        if isinstance(binding, BoolBinding):
            value = self.index(row, column).data(
                role=Qt.CheckStateRole) == Qt.Checked
        else:
            value = self.index(row, column).data(role=Qt.DisplayRole)
            if isinstance(binding, VectorBinding):
                value = string2list(value)
            value = binding.validate_trait("value", value)
        return key, value

    def get_header_key(self, section):
        """Retrieve the key of the binding for the section"""
        return self._header[section]

    def clear_model(self):
        """Clear the model and remove all data"""
        self.beginResetModel()
        try:
            self._data = []
        finally:
            self.endResetModel()

    def set_readonly(self, value):
        """Set the readonly role of the table element"""
        self._readonly = value

    # ------------------------------------------------------------------------
    # Additional methods

    def duplicate_row(self, pos):
        self.insertRows(pos + 1, 1, QModelIndex(), copy_row=self._data[pos])

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
