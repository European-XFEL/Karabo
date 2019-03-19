#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 22, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QAbstractTableModel, QModelIndex, Qt

from .const import (
    ALARM_DATA, ALARM_TYPE, REMOVE_ALARM_TYPES, UPDATE_ALARM_TYPES,
    get_alarm_icon, get_alarm_key_index)


class AlarmModel(QAbstractTableModel):
    """ A class which describes the relevant data (model) of a alarm service
    device to show in a table view.
    """
    headers = list(ALARM_DATA.values())

    def __init__(self, instanceId, parent=None):
        super(AlarmModel, self).__init__(parent)
        self.instanceId = instanceId  # InstanceId of associated AlarmService
        self.all_entries = []  # All alarm entries

    def initAlarms(self, instanceId, updateTypes, alarmEntries):
        if self.instanceId != instanceId:
            return
        self.beginResetModel()
        self.all_entries = alarmEntries
        self.endResetModel()

    def updateAlarms(self, instanceId, updateTypes, alarmEntries):
        if self.instanceId != instanceId:
            return
        # Insert updated entries in all entries list
        self.beginResetModel()

        for upType, alarmEntry in zip(updateTypes, alarmEntries):
            entryIndex = self._getEntryIndex(alarmEntry.id)
            if upType in UPDATE_ALARM_TYPES:
                if 0 <= entryIndex < len(self.all_entries):
                    # Replace entry
                    self.all_entries[entryIndex] = alarmEntry
                else:
                    self.all_entries.append(alarmEntry)
            elif upType in REMOVE_ALARM_TYPES:
                if self.all_entries:
                    self.all_entries.pop(entryIndex)
        self.endResetModel()

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

    def rowCount(self, parent=QModelIndex()):
        return len(self.all_entries)

    def columnCount(self, parent=QModelIndex()):
        return len(self.headers)

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None
        entry = self.all_entries[index.row()]
        type_index = get_alarm_key_index(ALARM_TYPE)
        if role in (Qt.DisplayRole, Qt.ToolTipRole):
            return entry[index.column()]
        elif role == Qt.DecorationRole and index.column() == type_index:
            return get_alarm_icon(entry.type)

        return None
