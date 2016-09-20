#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 16, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import namedtuple

from PyQt4.QtCore import (
    pyqtSlot, QAbstractTableModel, QDateTime, QModelIndex, Qt)
from PyQt4.QtGui import (
    QButtonGroup, QColor, QComboBox, QHBoxLayout, QLabel, QPixmap, QPushButton,
    QStyle, QStyledItemDelegate, QTableView, QVBoxLayout, QWidget)

from karabo_gui.docktabwindow import Dockable
from karabo_gui.mediator import (
    KaraboBroadcastEvent, KaraboEventSender, register_for_broadcasts)
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


updateType = ['init', 'add', 'remove', 'update', 'acknowledgeable',
              'deviceKilled', 'refuseAcknowledgement']


AlarmEntry = namedtuple('AlarmEntry', ['id',
                                       'timeOfFirstOccurrence',
                                       'timeOfOccurrence',
                                       'trainOfFirstOccurrence',
                                       'trainOfOccurrence',
                                       'deviceId',
                                       'property',
                                       'type',
                                       'description',
                                       'acknowledgeable',
                                       'needsAcknowledging' # Show Device
                                       ])


class AlarmServiceModel(QAbstractTableModel):
    headers = ['ID',
               'Time of First Occurence',
               'Time of Occurence',
               'Train of First Occurence',
               'Train of Occurence',
               'Device ID',
               'Property',
               'Type',
               'Description',
               'Acknowledge', # needsAcknowledging/acknowledgeable
               'Device']

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
                timeOfFirstOccurrence = Timestamp(alarmHash.get('timeOfFirstOccurrence')).toTimestamp()
                timeOfOccurrence = Timestamp(alarmHash.get('timeOfOccurrence')).toTimestamp()
                trainOfFirstOccurrence = alarmHash.get('trainOfFirstOccurrence')
                trainOfOccurrence = alarmHash.get('trainOfOccurrence')
                alarmEntry = AlarmEntry(
                    id=str(alarmHash.get('id')),
                    timeOfFirstOccurrence=QDateTime.fromMSecsSinceEpoch(timeOfFirstOccurrence * 1000),
                    timeOfOccurrence=QDateTime.fromMSecsSinceEpoch(timeOfOccurrence * 1000),
                    trainOfFirstOccurrence=str(trainOfFirstOccurrence),
                    trainOfOccurrence=str(trainOfOccurrence),
                    needsAcknowledging=alarmHash.get('needsAcknowledging'),
                    acknowledgeable=alarmHash.get('acknowledgeable'),
                    description=alarmHash.get('description'),
                    deviceId=alarmHash.get('deviceId'),
                    property=alarmHash.get('property'),
                    type=alarmHash.get('type'))
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
        #if role == Qt.DecorationRole and index.column() == 2:
        #    return self.icons.get(entry.messageType)
        if role == Qt.TextColorRole and index.column() == 7:
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

    def createEditor(self, parent, option, index):
        model = index.model()
        headerData = model.headerData(index.column(), Qt.Horizontal, Qt.DisplayRole)
        if headerData == 'Acknowledge' or headerData == 'Device':
            button = QPushButton(parent)
            self.pbClick.setText("Show {}".format(headerData))
            self.pbClick.setEnabled(True if index.data() else False)
            button.setFocusPolicy(Qt.NoFocus)
            return button
        else:
            return super(ButtonDelegate, self).createEditor(parent, option, index)

    def setEditorData(self, button, index):
        model = index.model()
        headerData = model.headerData(index.column(), Qt.Horizontal, Qt.DisplayRole)
        if headerData == 'Acknowledge' or headerData == 'Device':
            self.pbClick.setText("Show {}".format(headerData))
            self.pbClick.setEnabled(True if index.data() else False)
        else:
            super(ButtonDelegate, self).setEditorData(button, index)

    def paint(self, painter, option, index):
        model = index.model()
        headerData = model.headerData(index.column(), Qt.Horizontal, Qt.DisplayRole)
        if headerData == 'Acknowledge' or headerData == 'Device':
            self.pbClick.setGeometry(option.rect)
            self.pbClick.setText("Show {}".format(headerData))
            self.pbClick.setEnabled(True if index.data() else False)
            if option.state == QStyle.State_Selected:
                painter.fillRect(option.rect, option.palette.highlight())
            pixmap = QPixmap.grabWidget(self.pbClick)
            painter.drawPixmap(option.rect.x(), option.rect.y(), pixmap)
        else:
            super(ButtonDelegate, self).paint(painter, option, index)

    def updateEditorGeometry(self, button, option, index):
        button.setGeometry(option.rect)

    def setModelData(self, button, model, index):
        print("setModelData", button, model, index)

    @pyqtSlot(object)
    def cellClicked(self, index):
        model = index.model()
        headerData = model.headerData(index.column(), Qt.Horizontal, Qt.DisplayRole)
        print("cellClicked", headerData, index.data())
        if headerData == 'Acknowledge' or headerData == 'Device':
            if self.cellEditMode:
                self.parent().closePersistentEditor(self.currentCellIndex)
            self.parent().openPersistentEditor(index)
            self.cellEditMode = True
            self.currentCellIndex = index
        else:
            if self.cellEditMode:
                self.cellEditMode = False
                self.parent().closePersistentEditor(self.currentCellIndex)
