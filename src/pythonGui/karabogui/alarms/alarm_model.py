#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 22, 2016
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from qtpy.QtCore import QAbstractTableModel, QModelIndex, Qt

from karabo.common.api import KARABO_ALARM_SERVICE

from .const import (
    ALARM_DATA, ALARM_TYPE, REMOVE_ALARM_TYPES, UPDATE_ALARM_TYPES,
    get_alarm_icon, get_alarm_key_index)


class AlarmModel(QAbstractTableModel):
    """ A class which describes the relevant data (model) of a alarm service
    device to show in a table view.

    The AlarmModel represents a singleton in the karaboGUI as can be accessed
    via `get_alarm_model`.
    """
    headers = list(ALARM_DATA.values())

    def __init__(self, parent=None):
        super(AlarmModel, self).__init__(parent)
        self.instanceId = KARABO_ALARM_SERVICE
        self.all_entries = []  # All alarm entries

    def init_alarms_info(self, data):
        instanceId = data.get('instance_id')
        if self.instanceId != instanceId:
            return
        self.beginResetModel()
        self.all_entries = data.get('alarm_entries')
        self.endResetModel()

    def reset_alarms(self, instanceId):
        if self.instanceId != instanceId:
            return
        self.beginResetModel()
        self.all_entries = []
        self.endResetModel()

    def update_alarms_info(self, data):
        instanceId = data['instance_id']
        if self.instanceId != instanceId:
            return
        # Insert updated entries in all entries list
        update_types = data['update_types']
        alarm_entries = data['alarm_entries']
        for upType, alarmEntry in zip(update_types, alarm_entries):
            entryIndex = self._getEntryIndex(alarmEntry.id)
            if upType in UPDATE_ALARM_TYPES:
                if 0 <= entryIndex < len(self.all_entries):
                    # Replace entry, but do not announce a single dataChanged
                    self.all_entries[entryIndex] = alarmEntry
                else:
                    row = self.rowCount()
                    self.beginInsertRows(QModelIndex(), row, row)
                    self.all_entries.append(alarmEntry)
                    self.endInsertRows()
            elif upType in REMOVE_ALARM_TYPES:
                if self.all_entries and entryIndex != -1:
                    self.beginRemoveRows(QModelIndex(), entryIndex, entryIndex)
                    self.all_entries.pop(entryIndex)
                    self.endRemoveRows()

        rows = self.rowCount() - 1
        columns = self.columnCount() - 1
        first_index = self.index(0, 0)
        last_index = self.index(rows, columns)
        roles = [Qt.DisplayRole, Qt.ToolTipRole, Qt.DecorationRole]
        self.dataChanged.emit(first_index, last_index, roles)

    def _getEntryIndex(self, entry_id):
        """ The index in ``self.all_entries`` for the given ``entry_id`` is
            returned.
            If the ``entry_id`` is not found, ``-1`` is returned.
        """
        for index, entry in enumerate(self.all_entries):
            if entry.id == entry_id:
                return index
        return -1

    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return self.headers[section]

    def get_alarm_id(self, index):
        entry = self.all_entries[index.row()]
        return entry.id

    def rowCount(self, parent=QModelIndex()):
        return len(self.all_entries)

    def columnCount(self, parent=QModelIndex()):
        # We substract the id to show
        return len(self.headers) - 1

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None
        entry = self.all_entries[index.row()]
        type_index = get_alarm_key_index(ALARM_TYPE)
        if role in (Qt.DisplayRole, Qt.ToolTipRole):
            return entry[index.column()]
        if index.column() == type_index and role == Qt.DecorationRole:
            return get_alarm_icon(entry.type)

        return None
