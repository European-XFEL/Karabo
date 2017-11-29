#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 22, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QAbstractTableModel, QModelIndex, Qt

from .const import (ACKNOWLEDGE, ALARM_DATA, ALARM_TYPE, DEVICE_ID, PROPERTY,
                    REMOVE_ALARM_TYPES, UPDATE_ALARM_TYPES, get_alarm_icon,
                    get_alarm_key_index)


class AlarmModel(QAbstractTableModel):
    """ A class which describes the relevant data (model) of a alarm service
    device to show in a table view.
    """
    headers = list(ALARM_DATA.values())

    def __init__(self, instanceId, parent=None):
        super(AlarmModel, self).__init__(parent)
        self.instanceId = instanceId  # InstanceId of associated AlarmService
        self.allEntries = []  # All alarm entries
        self.filtered = []  # Filtered alarm entries
        self.filterSettings = {}  # Filter settings set by AlarmPanel

    def initAlarms(self, instanceId, updateTypes, alarmEntries):
        if self.instanceId != instanceId:
            return
        self.allEntries = alarmEntries
        self.updateFilter()

    def updateAlarms(self, instanceId, updateTypes, alarmEntries):
        if self.instanceId != instanceId:
            return
        # Insert updated entries in all entries list
        for upType, alarmEntry in zip(updateTypes, alarmEntries):
            entryIndex = self._getEntryIndex(alarmEntry.id)
            if upType in UPDATE_ALARM_TYPES:
                if 0 <= entryIndex < len(self.allEntries):
                    # Replace entry
                    self.allEntries[entryIndex] = alarmEntry
                else:
                    self.allEntries.append(alarmEntry)
            elif upType in REMOVE_ALARM_TYPES:
                if self.allEntries:
                    self.allEntries.pop(entryIndex)
        self.updateFilter()

    def updateFilter(self, **params):
        """ Fetch filtered data of all alarm entries.
            ``params`` is a dict which might include the keys:
            ``filterType`` which describes the filter
            ``text`` additional string for custom filtering
        """
        if params:
            self.filterSettings = params

        filterType = self.filterSettings.get('filterType', None)
        text = self.filterSettings.get('text', None)
        filtered = []
        if filterType is None:
            filtered = self.allEntries
        else:
            for entry in self.allEntries:
                if filterType == ACKNOWLEDGE:
                    needsAck, _ = entry.acknowledge
                    # Only check for ``needsAcknowledging`` flag
                    if needsAck:
                        filtered.append(entry)
                elif filterType == DEVICE_ID and text in entry.deviceId:
                        filtered.append(entry)
                elif filterType == PROPERTY and text in entry.property:
                        filtered.append(entry)
                elif filterType == ALARM_TYPE and text in entry.type:
                        filtered.append(entry)
        self._setFilterList(filtered)

    def _getEntryIndex(self, entry_id):
        """ The index in ``self.allEntries`` for the given ``entry_id`` is
            returned.
            If the ``entry_id`` is not found, ``-1`` is returned.
        """
        for index, entry in enumerate(self.allEntries):
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
