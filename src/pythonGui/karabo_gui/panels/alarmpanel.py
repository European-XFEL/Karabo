#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 16, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import namedtuple

from PyQt4.QtCore import QAbstractTableModel, Qt, QVariant
from PyQt4.QtGui import (
    QButtonGroup, QComboBox, QHBoxLayout, QLabel, QPushButton, QTableView,
    QVBoxLayout, QWidget)

from karabo_gui.docktabwindow import Dockable
from karabo_gui.mediator import (
    KaraboBroadcastEvent, KaraboEventSender, register_for_broadcasts)
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

        self.alarm_model = AlarmServiceModel()
        tw_alarms = QTableView()
        tw_alarms.setModel(self.alarm_model)

        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.addLayout(filter_layout)
        main_layout.addWidget(tw_alarms)

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
        self.alarm_model.initAlarms(instanceId, rows)

    def _updateAlarms(self, instanceId, rows):
        self.alarm_model.updateAlarms(instanceId, rows)


class AlarmServiceModel(QAbstractTableModel):
    headers = ['Time of First Occurence',
               'Time of Occurence',
               'Train of First Occurence',
               'Train of Occurence',
               'Device ID',
               'Property',
               'Type',
               'Description',
               'Acknowledge',
               'Show Device']

    #entries = ['timeOfFirstOccurrence',
    #           'timeOfOccurrence',
    #           'trainOfFirstOccurrence',
    #           'trainOfOccurrence',
    #           'needsAcknowledging',
    #           'acknowledgeable',
    #           'deviceId',
    #           'property',
    #           'type',
    #           'id']

    updateType = ['init', 'add', 'remove', 'update', 'acknowledgeable',
                  'deviceKilled', 'refuseAcknowledgement']

    AlarmEntry = namedtuple('AlarmEntry', ['timeOfFirstOccurrence',
                                           'timeOfOccurrence',
                                           'trainOfFirstOccurrence',
                                           'trainOfOccurrence',
                                           'needsAcknowledging',
                                           'acknowledgeable',
                                           'deviceId',
                                           'property',
                                           'type',
                                           'id'])

    def __init__(self, parent=None):
        super(AlarmServiceModel, self).__init__(parent)

    def initAlarms(self, instanceId, rows):
        print("+++ AlarmModel.initAlarms", instanceId)
        alarm_list = []
        for id, h, _ in rows.iterall():
            print("rowId", id)
            # Get data of hash
            for updateType, alarmHash, _ in h.iterall():
                print("updateType:", updateType)
                print()
                alarmEntry = {}
                for key, entry, _ in alarmHash.iterall():
                    print("---", key, entry)
                    alarmEntry[key] = entry
                print("FINAL", alarmEntry)
                alarm_list.append(alarmEntry)
            print()
        print(alarm_list)
        self.beginResetModel()
        self.filtered = alarm_list
        self.endResetModel()

    def updateAlarms(self, instanceId, rows):
        print("AlarmModel.updateAlarms", instanceId)
        #for k, v, _ in rows.iterall():
        #    print("....k, v, attrs...")
        #    print(k)
        #    print(v)
        #    print()
        #print()

    def insert(self, data):
        print("insert", data)
        return
        hi = len(self.filtered)
        lo = 0
        key = self.key(data)
        while hi > lo:
            mid = (hi + lo) // 2
            if self.reverse == (self.key(self.filtered[mid]) < key):
                hi = mid
            else:
                lo = mid + 1
        self.beginInsertRows(QModelIndex(), lo, lo + 1)
        self.filtered.insert(lo, data)
        self.endInsertRows()

    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return self.headers[section]

    def rowCount(self, _):
        return 0

    def columnCount(self, _):
        return len(self.headers)

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None

    def insertRows(self):
        pass

    def removeRows(self):
        pass
