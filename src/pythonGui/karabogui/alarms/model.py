#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 22, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QAbstractTableModel, QModelIndex, Qt

from .const import (
    ALARM_DATA, ALARM_TYPE, ALARM_WARNING_TYPES, REMOVE_ALARM_TYPES,
    UPDATE_ALARM_TYPES, get_alarm_icon, get_alarm_key_index)


class AlarmModel(QAbstractTableModel):
    """ A class which describes the relevant data (model) of a alarm service
    device to show in a table view.
    """
    headers = list(ALARM_DATA.values())

    def __init__(self, instanceId, parent=None):
        super(AlarmModel, self).__init__(parent)
        self.instanceId = instanceId  # InstanceId of associated AlarmService
        self.all_entries = []  # All alarm entries
        self.filtered = []  # Filtered alarm entries
        self.filter_type = ALARM_WARNING_TYPES  # default filter

    def initAlarms(self, instanceId, updateTypes, alarmEntries):
        if self.instanceId != instanceId:
            return
        self.all_entries = alarmEntries
        self.updateFilter()

    def updateAlarms(self, instanceId, updateTypes, alarmEntries):
        if self.instanceId != instanceId:
            return
        # Insert updated entries in all entries list
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
        self.updateFilter()

    def updateFilter(self, filter_type=None):
        """ Fetch filtered data of all alarm entries.

            :param filter_type: An optionally new filter set by the panel
        """
        if filter_type is not None:
            self.filter_type = filter_type
        filtered = []
        for entry in self.all_entries:
            alarm_type = entry.type
            if alarm_type in self.filter_type:
                filtered.append(entry)
        self._setFilterList(filtered)

    def _getEntryIndex(self, entry_id):
        """ The index in ``self.all_entries`` for the given ``entry_id`` is
            returned.
            If the ``entry_id`` is not found, ``-1`` is returned.
        """
        for index, entry in enumerate(self.all_entries):
            if entry.id == entry_id:
                return index
        return -1

    def _setFilterList(self, filtered):
        """ Update filter list and reset model."""
        self.beginResetModel()
        self.filtered = filtered
        self.endResetModel()

    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return self.headers[section]

    def rowCount(self, parent=QModelIndex()):
        return len(self.filtered)

    def columnCount(self, parent=QModelIndex()):
        return len(self.headers)

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None
        entry = self.filtered[index.row()]
        type_index = get_alarm_key_index(ALARM_TYPE)
        if role == Qt.DecorationRole and index.column() == type_index:
            return get_alarm_icon(entry.type)
        elif role in (Qt.DisplayRole, Qt.ToolTipRole):
            return entry[index.column()]
        return None
