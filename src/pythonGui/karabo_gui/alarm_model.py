#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 22, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import OrderedDict, namedtuple

from PyQt4.QtCore import QAbstractTableModel, QDateTime, QModelIndex, Qt

from karabo.middlelayer import Timestamp
from karabo_gui.const import ALARM_COLOR, WARN_COLOR

ALARM_ID = 'id'
TIME_OF_FIRST_OCCURENCE = 'timeOfFirstOccurrence'
TIME_OF_OCCURENCE = 'timeOfOccurrence'
TRAIN_OF_FIRST_OCCURENCE = 'trainOfFirstOccurrence'
TRAIN_OF_OCCURENCE = 'trainOfOccurrence'
DEVICE_ID = 'deviceId'
PROPERTY = 'property'
ALARM_TYPE = 'type'
DESCRIPTION = 'description'
NEEDS_ACKNOWLEDGING = 'needsAcknowledging'
ACKNOWLEDGEABLE = 'acknowledgeable'
ACKNOWLEDGE = 'acknowledge'  # puts together needsAcknowledging/acknowledgeable
SHOW_DEVICE = 'showDevice'

ALARM_DATA = OrderedDict()
ALARM_DATA[ALARM_ID] = 'ID'
ALARM_DATA[TIME_OF_FIRST_OCCURENCE] = 'Time of First Occurence'
ALARM_DATA[TIME_OF_OCCURENCE] = 'Time of Occurence'
ALARM_DATA[TRAIN_OF_FIRST_OCCURENCE] = 'Train of First Occurence'
ALARM_DATA[TRAIN_OF_OCCURENCE] = 'Train of Occurence'
ALARM_DATA[DEVICE_ID] = 'Device ID'
ALARM_DATA[PROPERTY] = 'Property'
ALARM_DATA[ALARM_TYPE] = 'Type'
ALARM_DATA[DESCRIPTION] = 'Description'
ALARM_DATA[ACKNOWLEDGE] = 'Acknowledge'
ALARM_DATA[SHOW_DEVICE] = 'Show Device'

AlarmEntry = namedtuple('AlarmEntry', [key for key in ALARM_DATA.keys()])

INIT_UPDATE_TYPE = 'init'
ADD_UPDATE_TYPE = 'add'
REMOVE_UPDATE_TYPE = 'remove'
UPDATE_UPDATE_TYPE = 'update'
ACKNOWLEGDABLE_UPDATE_TYPE = 'acknowledgeable'
DEVICE_KILLED_UPDATE_TYPE = 'deviceKilled'
REFUSE_ACKNOWLEDGEMENT_UPDATE_TYPE = 'refuseAcknowledgement'

# Tuples for convenience
UPDATE_ALARM = (INIT_UPDATE_TYPE, UPDATE_UPDATE_TYPE, ADD_UPDATE_TYPE,
                ACKNOWLEGDABLE_UPDATE_TYPE, REFUSE_ACKNOWLEDGEMENT_UPDATE_TYPE)
REMOVE_ALARM = (REMOVE_UPDATE_TYPE, DEVICE_KILLED_UPDATE_TYPE)


def getAlarmKeyIndex(key):
    """ Return ``index`` position in ``ALARM_DATA`` OrderedDict for the given
        ``key``.
        If the ``key`` is not found, ``None`` is returned."""
    return list(ALARM_DATA.keys()).index(key)


class AlarmModel(QAbstractTableModel):
    """ A class which describes the relevant data (model) of a alarm service
        device to show in a table view. """
    headers = [value for key, value in ALARM_DATA.items()]

    textColor = {'warnLow': WARN_COLOR,
                 'warnHigh': WARN_COLOR,
                 'alarmLow': ALARM_COLOR,
                 'alarmHigh': ALARM_COLOR}

    def __init__(self, parent=None):
        super(AlarmModel, self).__init__(parent)
        self.instanceId = ""  # InstanceId of the associated AlarmService-Device
        self.filtered = []

    def extractData(self, rows):
        """ Fetch data from incoming hash object ``rows`` and put
            ``updateTypes`` and all ``alarmEntries`` into lists and return
            them.
        """
        updateTypes = []
        alarmEntries = []
        for id, h, _ in rows.iterall():
            # Get data of hash
            for updateType, alarmHash, _ in h.iterall():
                updateTypes.append(updateType)
                # XXX: TODO use proper UTC to local time lib
                timeOfFirstOccurrence = Timestamp(alarmHash.get(TIME_OF_FIRST_OCCURENCE)).toTimestamp()
                timeOfOccurrence = Timestamp(alarmHash.get(TIME_OF_OCCURENCE)).toTimestamp()
                trainOfFirstOccurrence = alarmHash.get(TRAIN_OF_FIRST_OCCURENCE)
                trainOfOccurrence = alarmHash.get(TRAIN_OF_OCCURENCE)
                needsAck = alarmHash.get(NEEDS_ACKNOWLEDGING)
                ack = alarmHash.get(ACKNOWLEDGEABLE)
                deviceId = alarmHash.get(DEVICE_ID)
                alarmEntry = AlarmEntry(
                    id=str(alarmHash.get(ALARM_ID)),
                    timeOfFirstOccurrence=QDateTime.fromMSecsSinceEpoch(timeOfFirstOccurrence * 1000),
                    timeOfOccurrence=QDateTime.fromMSecsSinceEpoch(timeOfOccurrence * 1000),
                    trainOfFirstOccurrence=str(trainOfFirstOccurrence),
                    trainOfOccurrence=str(trainOfOccurrence),
                    deviceId=deviceId,
                    property=alarmHash.get(PROPERTY),
                    type=alarmHash.get(ALARM_TYPE),
                    description=alarmHash.get(DESCRIPTION),
                    acknowledge=(needsAck, ack),
                    showDevice=deviceId,
                    )
                alarmEntries.append(alarmEntry)
        return updateTypes, alarmEntries

    def initAlarms(self, instanceId, rows):
        self.instanceId = instanceId
        _, alarmEntries = self.extractData(rows)
        self.beginResetModel()
        self.filtered = alarmEntries
        self.endResetModel()

    def updateAlarms(self, instanceId, rows):
        updateTypes, alarmEntries = self.extractData(rows)
        for upType, alarmEntry in zip(updateTypes, alarmEntries):
            rowIndex = self._getRowIndexFromId(alarmEntry.id)
            if upType in UPDATE_ALARM:
                if rowIndex > -1 and rowIndex < len(self.filtered):
                    # Remove old entry from list first
                    self.removeRow(rowIndex)
                    self.insertRow(rowIndex, alarmEntry)
                else:
                    self.insertRow(len(self.filtered), alarmEntry)
            elif upType in REMOVE_ALARM:
                self.removeRow(rowIndex)

    def _getRowIndexFromId(self, id):
        """ The row index for the given ``id`` is returned.
            If the ``id`` is not found, ``-1`` is returned.
        """
        for row in range(self.rowCount()):
            index = self.index(row, getAlarmKeyIndex(ALARM_ID))
            if index.data() == id:
                return row
        return -1

    def insertRow(self, index, alarmEntry):
        self.beginInsertRows(QModelIndex(), index, index)
        self.filtered.insert(index, alarmEntry)
        self.endInsertRows()

    def removeRow(self, index):
        self.beginRemoveRows(QModelIndex(), index, index)
        self.filtered.pop(index)
        self.endRemoveRows()

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
        type_index = getAlarmKeyIndex(ALARM_TYPE)
        # XXX: TODO: Handle icons display
        if role == Qt.TextColorRole and index.column() == type_index:
            return self.textColor.get(entry.type)
        elif role in (Qt.DisplayRole, Qt.ToolTipRole):
            return entry[index.column()]
        return None
