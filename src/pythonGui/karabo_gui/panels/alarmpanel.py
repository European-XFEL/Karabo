#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 16, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import namedtuple, OrderedDict

from PyQt4.QtCore import (
    pyqtSlot, QAbstractTableModel, QDateTime, QModelIndex, Qt)
from PyQt4.QtGui import (
    QButtonGroup, QColor, QComboBox, QHBoxLayout, QLabel, QPixmap, QPushButton,
    QStyle, QStyledItemDelegate, QTableView, QVBoxLayout, QWidget)

from karabo_gui.docktabwindow import Dockable
from karabo_gui.mediator import (
    broadcast_event, KaraboBroadcastEvent, KaraboEventSender,
    register_for_broadcasts)
from karabo.middlelayer import Timestamp
from karabo_gui.network import Network

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

_ALARM_DATA = OrderedDict()
_ALARM_DATA[ALARM_ID] = 'ID'
_ALARM_DATA[TIME_OF_FIRST_OCCURENCE] = 'Time of First Occurence'
_ALARM_DATA[TIME_OF_OCCURENCE] = 'Time of Occurence'
_ALARM_DATA[TRAIN_OF_FIRST_OCCURENCE] = 'Train of First Occurence'
_ALARM_DATA[TRAIN_OF_OCCURENCE] = 'Train of Occurence'
_ALARM_DATA[DEVICE_ID] = 'Device ID'
_ALARM_DATA[PROPERTY] = 'Property'
_ALARM_DATA[ALARM_TYPE] = 'Type'
_ALARM_DATA[DESCRIPTION] = 'Description'
_ALARM_DATA[ACKNOWLEDGE] = 'Acknowledge'
_ALARM_DATA[SHOW_DEVICE] = 'Show Device'


AlarmEntry = namedtuple('AlarmEntry', [key for key in _ALARM_DATA.keys()])


INIT_UPDATE_TYPE = 'init'
ADD_UPDATE_TYPE = 'add'
REMOVE_UPDATE_TYPE = 'remove'
UPDATE_UPDATE_TYPE = 'update'
ACKNOWLEGDABLE_UPDATE_TYPE = 'acknowledgeable'
DEVICE_KILLED_UPDATE_TYPE = 'deviceKilled'
REFUSE_ACKNOWLEDGEMENT_UPDATE_TYPE = 'refuseAcknowledgement'


def get_alarm_key_index(key):
    """ Return ``index`` position in ``_ALARM_DATA`` OrderedDict for the given
        ``key``.
        If the ``key`` is not found, ``None`` is returned."""
    index = -1
    for k in _ALARM_DATA.keys():
        index = index + 1
        if k == key:
            return index


class AlarmPanel(Dockable, QWidget):
    def __init__(self):
        super(AlarmPanel, self).__init__()

        self.bg_filter = QButtonGroup()
        pb_default_view = QPushButton("Default view")
        pb_default_view.setCheckable(True)
        pb_default_view.setChecked(True)
        self.bg_filter.addButton(pb_default_view)
        pb_acknowledge_only = QPushButton("Acknowledge only")
        pb_acknowledge_only.setCheckable(True)
        self.bg_filter.addButton(pb_acknowledge_only)

        la_filter_options = QLabel("Filter options")
        cb_filter_type = QComboBox()
        cb_filter_type.addItem("Device ID")
        cb_filter_type.addItem("Type")
        cb_filter_type.addItem("Class")
        pb_custom_filter = QPushButton("Filter")
        pb_custom_filter.setCheckable(True)
        self.bg_filter.addButton(pb_custom_filter)

        filter_layout = QHBoxLayout()
        filter_layout.setContentsMargins(0, 0, 0, 0)
        filter_layout.addWidget(pb_default_view)
        filter_layout.addWidget(pb_acknowledge_only)
        # Add custom filter options
        filter_layout.addWidget(la_filter_options)
        filter_layout.addWidget(cb_filter_type)
        filter_layout.addWidget(pb_custom_filter)
        filter_layout.addStretch()

        self.twAlarm = QTableView()
        self.twAlarm.setWordWrap(True)
        self.twAlarm.setAlternatingRowColors(True)
        self.twAlarm.resizeColumnsToContents()
        self.twAlarm.horizontalHeader().setStretchLastSection(True)
        alarm_model = AlarmServiceModel()
        self.twAlarm.setModel(alarm_model)
        btn_delegate = ButtonDelegate(self.twAlarm)
        self.twAlarm.setItemDelegate(btn_delegate)

        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.addLayout(filter_layout)
        main_layout.addWidget(self.twAlarm)

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary for self due to the fact that the singleton mediator
        # object and `self` are being destroyed when the GUI exists
        register_for_broadcasts(self)

    def eventFilter(self, obj, event):
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.AlarmInitReply:
                data = event.data
                self._initAlarms(data.get('instanceId'), data.get('rows'))
                return True
            elif event.sender is KaraboEventSender.AlarmUpdate:
                data = event.data
                self._updateAlarms(data.get('instanceId'), data.get('rows'))
                return True
        return super(AlarmPanel, self).eventFilter(obj, event)

    def setupActions(self):
        pass

    def setupToolBars(self, toolBar, parent):
        pass

    def setEnabled(self, enable):
        if enable:
            Network().onRequestAlarms()
        super(AlarmPanel, self).setEnabled(enable)

    def _initAlarms(self, instanceId, rows):
        self.twAlarm.model().initAlarms(instanceId, rows)

    def _updateAlarms(self, instanceId, rows):
        self.twAlarm.model().updateAlarms(instanceId, rows)


class AlarmServiceModel(QAbstractTableModel):
    """ A class which describes the relevant data (model) of a alarm service
        device to show in a table view. """
    headers = [value for key, value in _ALARM_DATA.items()]

    textColor = {'warnLow': QColor(255, 102, 0),
                 'warnHigh': QColor(255, 102, 0),
                 'alarmLow': QColor(255, 204, 102),
                 'alarmHigh': QColor(255, 204, 102)
                }

    def __init__(self, parent=None):
        super(AlarmServiceModel, self).__init__(parent)
        self.filtered = []

    def fetchData(self, rows):
        """ Fetch data from incoming hash object ``rows`` and put
            ``updateTypes`` and all ``alarmEntries`` into lists and return them.
        """
        updateTypes = []
        alarmEntries = []
        for id, h, _ in rows.iterall():
            # Get data of hash
            for updateType, alarmHash, _ in h.iterall():
                updateTypes.append(updateType)
                timeOfFirstOccurrence = Timestamp(alarmHash.get(TIME_OF_FIRST_OCCURENCE)).toTimestamp()
                timeOfOccurrence = Timestamp(alarmHash.get(TIME_OF_OCCURENCE)).toTimestamp()
                trainOfFirstOccurrence = alarmHash.get(TRAIN_OF_FIRST_OCCURENCE)
                trainOfOccurrence = alarmHash.get(TRAIN_OF_OCCURENCE)
                needsAck = alarmHash.get(NEEDS_ACKNOWLEDGING)
                acknowledge = alarmHash.get(ACKNOWLEDGEABLE)
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
                    acknowledge=(needsAck, acknowledge),
                    showDevice=deviceId,
                    )
                alarmEntries.append(alarmEntry)
        return updateTypes, alarmEntries

    def initAlarms(self, instanceId, rows):
        print()
        print("+++ AlarmModel.initAlarms", instanceId)
        _, alarmEntries = self.fetchData(rows)
        self.beginResetModel()
        self.filtered = alarmEntries
        self.endResetModel()

    def updateAlarms(self, instanceId, rows):
        print()
        print("+++ AlarmModel.updateAlarms", instanceId)
        updateTypes, alarmEntries = self.fetchData(rows)
        for i, upType in enumerate(updateTypes):
            alarmEntry = alarmEntries[i]
            id = int(alarmEntry.id)
            # XXX: Use real row index here not id to insert/remove
            print("updateTypes", upType, id)
            if (upType == INIT_UPDATE_TYPE or upType == UPDATE_UPDATE_TYPE or
                upType == ADD_UPDATE_TYPE or
                upType == ACKNOWLEGDABLE_UPDATE_TYPE or
                upType == REFUSE_ACKNOWLEDGEMENT_UPDATE_TYPE):
                if id < len(self.filtered):
                    # Remove old entry from list
                    self.removeRow(id)
                self.insertRow(id, alarmEntry)
            elif (upType == REMOVE_UPDATE_TYPE or
                  upType == DEVICE_KILLED_UPDATE_TYPE):
                self.removeRow(id)

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

    def rowCount(self, _):
        return len(self.filtered)

    def columnCount(self, _):
        return len(self.headers)

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None
        entry = self.filtered[index.row()]
        type_index = get_alarm_key_index(ALARM_TYPE)
        #if role == Qt.DecorationRole and index.column() == 2:
        #    return self.icons.get(entry.messageType)
        if role == Qt.TextColorRole and index.column() == type_index:
            return self.textColor.get(entry.type)
        elif role in (Qt.DisplayRole, Qt.ToolTipRole):
            return entry[index.column()]
        return None


class ButtonDelegate(QStyledItemDelegate):
    def __init__(self, parent=None):
        super(ButtonDelegate, self).__init__(parent)
        # Fake button used for later rendering
        self.pbClick = QPushButton("", parent)
        self.pbClick.hide()
        parent.clicked.connect(self.cellClicked)
        self.cellEditMode = False
        self.currentCellIndex = None  # QPersistentModelIndex

    def _isRelevantColumn(self, index):
        """ This methods checks whether the column of the given ``index`` is
            belongs to either the acknowledging or device column.

            Returns tuple:
            [0] - states whether this is a relevant column
            [1] - the text which needs to be shown on the button
            Otherwise ``False`` is returned.
        """
        column = index.column()
        ack_index = get_alarm_key_index(ACKNOWLEDGE)
        device_index = get_alarm_key_index(SHOW_DEVICE)
        if column == ack_index or column == device_index:
            if column == ack_index:
                text = _ALARM_DATA[ACKNOWLEDGE]
            else:
                text = _ALARM_DATA[SHOW_DEVICE]
            return (True, text)
        return (False, '')

    def _updateButton(self, button, index):
        """ Set the visibility and enabling of the button depending on the
            properties ``NEEDS_ACKNOWLEDGING`` and ``ACKNOWLEDGEABLE``.
        """
        if index.column() == get_alarm_key_index(ACKNOWLEDGE):
            needsAck, ack = index.data()
            button.setEnabled(True if needsAck and ack else False)

    def createEditor(self, parent, option, index):
        isRelevant, text = self._isRelevantColumn(index)
        if isRelevant:
            button = QPushButton(parent)
            button.setText(text)
            self._updateButton(button, index)
            return button
        else:
            return super(ButtonDelegate, self).createEditor(parent, option, index)

    def setEditorData(self, button, index):
        isRelevant, text = self._isRelevantColumn(index)
        if isRelevant:
            button.setText(text)
            self._updateButton(button, index)
        else:
            super(ButtonDelegate, self).setEditorData(button, index)

    def paint(self, painter, option, index):
        isRelevant, text = self._isRelevantColumn(index)
        if isRelevant:
            self.pbClick.setGeometry(option.rect)
            self.pbClick.setText(text)
            self._updateButton(self.pbClick, index)
            if option.state == QStyle.State_Selected:
                painter.fillRect(option.rect, option.palette.highlight())
            pixmap = QPixmap.grabWidget(self.pbClick)
            painter.drawPixmap(option.rect.x(), option.rect.y(), pixmap)
        else:
            super(ButtonDelegate, self).paint(painter, option, index)

    def updateEditorGeometry(self, button, option, index):
        isRelevant, text = self._isRelevantColumn(index)
        if isRelevant:
            button.setGeometry(option.rect)
            button.setText(text)
            self._updateButton(button, index)

    @pyqtSlot(object)
    def cellClicked(self, index):
        isRelevant, text = self._isRelevantColumn(index)
        if isRelevant:
            if self.cellEditMode:
                self.parent().closePersistentEditor(self.currentCellIndex)
            self.parent().openPersistentEditor(index)
            self.cellEditMode = True
            self.currentCellIndex = index
            if text == _ALARM_DATA[SHOW_DEVICE]:
                # Send signal to show device
                deviceId = index.data()
                data = {'deviceId': deviceId}
                # Create KaraboBroadcastEvent
                broadcast_event(KaraboBroadcastEvent(
                    KaraboEventSender.ShowDevice, data))
            else:
                # Send signal to acknowledge alarm
                id_index = get_alarm_key_index(ALARM_ID)
                model = index.model()
                alarm_id = model.index(index.row(), id_index).data()
                Network().onAcknowledgeAlarm('Karabo_AlarmService_0', alarm_id)
        else:
            if self.cellEditMode:
                self.cellEditMode = False
                self.parent().closePersistentEditor(self.currentCellIndex)
