#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 16, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import namedtuple
from enum import Enum

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


class AlarmColumnData(Enum):
    """ Describes the indices of the tuple of the ``AlarmColumn`` Enum."""
    index = 0
    entry = 1
    entry_display = 2


class AlarmColumn(Enum):
    """ Describes possible alarm entries in a tuple.
        First entry describes the column of the entry in the table.
        Second entry describes the entry name in the hash.
        Thirtd Entry describes the display name of the entry in the table.
    """
    id = (0, 'id', 'ID')
    timeOfFirstOccurrence = (1, 'timeOfFirstOccurrence', 'Time of First Occurence')
    timeOfOccurrence = (2, 'timeOfOccurrence', 'Time of Occurence')
    trainOfFirstOccurrence = (3, 'trainOfFirstOccurrence', 'Train of First Occurence')
    trainOfOccurrence = (4, 'trainOfOccurrence', 'Train of Occurence')
    deviceId = (5, 'deviceId', 'Device ID')
    property = (6, 'property', 'Property')
    typ = (7, 'type', 'Type')
    description = (8, 'description', 'Description')
    needsAcknowledging = (9, 'needsAcknowledging', 'Acknowledge')
    acknowledgeable = (10, 'acknowledgeable', 'Acknowledge')


AlarmEntry = namedtuple(
    'AlarmEntry', [AlarmColumn.id.value[AlarmColumnData.entry.value],
                   AlarmColumn.timeOfFirstOccurrence.value[AlarmColumnData.entry.value],
                   AlarmColumn.timeOfOccurrence.value[AlarmColumnData.entry.value],
                   AlarmColumn.trainOfFirstOccurrence.value[AlarmColumnData.entry.value],
                   AlarmColumn.trainOfOccurrence.value[AlarmColumnData.entry.value],
                   AlarmColumn.deviceId.value[AlarmColumnData.entry.value],
                   AlarmColumn.property.value[AlarmColumnData.entry.value],
                   AlarmColumn.typ.value[AlarmColumnData.entry.value],
                   AlarmColumn.description.value[AlarmColumnData.entry.value],
                   AlarmColumn.needsAcknowledging.value[AlarmColumnData.entry.value],
                   AlarmColumn.acknowledgeable.value[AlarmColumnData.entry.value],
                   ])


updateType = ['init', 'add', 'remove', 'update', 'acknowledgeable',
              'deviceKilled', 'refuseAcknowledgement']


class AlarmServiceModel(QAbstractTableModel):
    """ A class which describes the relevant data (model) of a alarm service
        device to show in a table view. """
    headers = [AlarmColumn.id.value[AlarmColumnData.entry_display.value],
               AlarmColumn.timeOfFirstOccurrence.value[AlarmColumnData.entry_display.value],
               AlarmColumn.timeOfOccurrence.value[AlarmColumnData.entry_display.value],
               AlarmColumn.trainOfFirstOccurrence.value[AlarmColumnData.entry_display.value],
               AlarmColumn.trainOfOccurrence.value[AlarmColumnData.entry_display.value],
               AlarmColumn.deviceId.value[AlarmColumnData.entry_display.value],
               AlarmColumn.property.value[AlarmColumnData.entry_display.value],
               AlarmColumn.typ.value[AlarmColumnData.entry_display.value],
               AlarmColumn.description.value[AlarmColumnData.entry_display.value],
               AlarmColumn.needsAcknowledging.value[AlarmColumnData.entry_display.value],
               AlarmColumn.deviceId.value[AlarmColumnData.entry_display.value]
               ]

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
                id_entry = AlarmColumn.id.value[AlarmColumnData.entry.value]
                time_first = AlarmColumn.timeOfFirstOccurrence.value[AlarmColumnData.entry.value]
                time = AlarmColumn.timeOfOccurrence.value[AlarmColumnData.entry.value]
                train_first = AlarmColumn.trainOfFirstOccurrence.value[AlarmColumnData.entry.value]
                train = AlarmColumn.trainOfOccurrence.value[AlarmColumnData.entry.value]
                deviceId = AlarmColumn.deviceId.value[AlarmColumnData.entry.value]
                prop = AlarmColumn.property.value[AlarmColumnData.entry.value]
                typ = AlarmColumn.typ.value[AlarmColumnData.entry.value]
                description = AlarmColumn.description.value[AlarmColumnData.entry.value]
                needsAck = AlarmColumn.needsAcknowledging.value[AlarmColumnData.entry.value]
                ack = AlarmColumn.acknowledgeable.value[AlarmColumnData.entry.value]

                timeOfFirstOccurrence = Timestamp(alarmHash.get(time_first)).toTimestamp()
                timeOfOccurrence = Timestamp(alarmHash.get(time)).toTimestamp()
                trainOfFirstOccurrence = alarmHash.get(train_first)
                trainOfOccurrence = alarmHash.get(train)
                alarmEntry = AlarmEntry(
                    id=str(alarmHash.get(id_entry)),
                    timeOfFirstOccurrence=QDateTime.fromMSecsSinceEpoch(timeOfFirstOccurrence * 1000),
                    timeOfOccurrence=QDateTime.fromMSecsSinceEpoch(timeOfOccurrence * 1000),
                    trainOfFirstOccurrence=str(trainOfFirstOccurrence),
                    trainOfOccurrence=str(trainOfOccurrence),
                    needsAcknowledging=alarmHash.get(needsAck),
                    acknowledgeable=alarmHash.get(ack),
                    description=alarmHash.get(description),
                    deviceId=alarmHash.get(deviceId),
                    property=alarmHash.get(prop),
                    type=alarmHash.get(typ),
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
            if (upType == 'init' or upType == 'update' or upType == 'add' or
                upType == 'acknowledgeable' or upType == 'refuseAcknowledgement'):
                if id < len(self.filtered):
                    # Remove old entry from list
                    self.removeRow(id)
                self.insertRow(id, alarmEntry)
            elif upType == 'remove' or upType == 'deviceKilled':
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
        typ_index = AlarmColumn.typ.value[AlarmColumnData.index.value]
        #if role == Qt.DecorationRole and index.column() == 2:
        #    return self.icons.get(entry.messageType)
        if role == Qt.TextColorRole and index.column() == typ_index:
            return self.textColor.get(entry.type)
        elif role in (Qt.DisplayRole, Qt.ToolTipRole):
            column = index.column()
            needs_ack_index = AlarmColumn.needsAcknowledging.value[AlarmColumnData.index.value]
            ack_index = AlarmColumn.acknowledgeable.value[AlarmColumnData.index.value]
            if column == needs_ack_index:
                return (entry[column], entry[column+1])
            elif column == ack_index:
                deviceId_index = AlarmColumn.deviceId.value[AlarmColumnData.index.value]
                return entry[deviceId_index]
            else:
                return entry[index.column()]
        return None


class ButtonDelegate(QStyledItemDelegate):
    SHOW_DEVICE = "Show Device"

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
        ack_index = AlarmColumn.needsAcknowledging.value[AlarmColumnData.index.value]
        device_index = AlarmColumn.acknowledgeable.value[AlarmColumnData.index.value]
        if column == ack_index or column == device_index:
            if column == ack_index:
                text = AlarmColumn.acknowledgeable.value[AlarmColumnData.entry_display.value]
            else:
                text = self.SHOW_DEVICE
            return (True, text)
        return (False, '')

    def createEditor(self, parent, option, index):
        isRelevant, text = self._isRelevantColumn(index)
        if isRelevant:
            button = QPushButton(parent)
            self.pbClick.setText(text)
            self.pbClick.setEnabled(True if index.data() else False)
            #button.setFocusPolicy(Qt.NoFocus)
            return button
        else:
            return super(ButtonDelegate, self).createEditor(parent, option, index)

    def setEditorData(self, button, index):
        isRelevant, text = self._isRelevantColumn(index)
        if isRelevant:
            self.pbClick.setText(text)
            self.pbClick.setEnabled(True if index.data() else False)
        else:
            super(ButtonDelegate, self).setEditorData(button, index)

    def paint(self, painter, option, index):
        isRelevant, text = self._isRelevantColumn(index)
        if isRelevant:
            self.pbClick.setGeometry(option.rect)
            self.pbClick.setText(text)
            self.pbClick.setEnabled(True if index.data() else False)
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
            button.setEnabled(True if index.data() else False)

    @pyqtSlot(object)
    def cellClicked(self, index):
        isRelevant, text = self._isRelevantColumn(index)
        if isRelevant:
            if self.cellEditMode:
                self.parent().closePersistentEditor(self.currentCellIndex)
            self.parent().openPersistentEditor(index)
            self.cellEditMode = True
            self.currentCellIndex = index
            model = index.model()
            if text == self.SHOW_DEVICE:
                # Send signal to show device
                deviceId_index = AlarmColumn.deviceId.value[AlarmColumnData.index.value]
                deviceId = model.index(index.row(), deviceId_index).data()
                data = {'deviceId': deviceId}
                # Create KaraboBroadcastEvent
                broadcast_event(KaraboBroadcastEvent(
                    KaraboEventSender.ShowDevice, data))
            else:
                # Send signal to acknowledge alarm
                id_index = AlarmColumn.id.value[AlarmColumnData.index.value]
                id = model.index(index.row(), id_index).data()
                Network().onAcknowledgeAlarm('Karabo_AlarmService_0', id)
        else:
            if self.cellEditMode:
                self.cellEditMode = False
                self.parent().closePersistentEditor(self.currentCellIndex)
