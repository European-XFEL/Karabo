#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 4, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import namedtuple

from PyQt5.QtCore import (pyqtSlot, QAbstractTableModel, QDate, QDateTime,
                          QModelIndex, Qt)
from PyQt5.QtGui import QColor
from PyQt5.QtWidgets import (QAbstractItemView, QDateTimeEdit,
                             QFormLayout, QFrame, QGroupBox, QHBoxLayout,
                             QLabel, QLineEdit, QPushButton, QTableView,
                             QToolButton, QVBoxLayout, QWidget)

from karabogui import icons
from karabogui.events import broadcast_event, KaraboEvent
from karabogui.util import getSaveFileName

DEVICE_COLUMN = 3


class LogWidget(QWidget):
    def __init__(self, parent=None):
        # parent - parent widget
        super(LogWidget, self).__init__(parent)

        # Main layout
        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(0, 0, 0, 0)

        # Add button to collapse/expand filter options
        text = "Show filter options"
        self.pbFilterOptions = QPushButton("+ {}".format(text))
        self.pbFilterOptions.setStatusTip(text)
        self.pbFilterOptions.setToolTip(text)
        self.pbFilterOptions.setCheckable(True)
        self.pbFilterOptions.setChecked(False)

        font = self.pbFilterOptions.font()
        font.setBold(True)
        self.pbFilterOptions.setFont(font)
        self.pbFilterOptions.clicked.connect(self.onFilterOptionVisible)
        vLayout.addWidget(self.pbFilterOptions)

        # Create widget with filter options
        self.filterWidget = self._setupFilterWidget()
        vLayout.addWidget(self.filterWidget)

        self.logs = []
        self.tailindex = 0
        self.queryModel = LogQueryModel()
        twLogTable = QTableView(self)
        vLayout.addWidget(twLogTable)

        twLogTable.setModel(self.queryModel)
        twLogTable.setWordWrap(True)
        twLogTable.setAlternatingRowColors(True)
        twLogTable.horizontalHeader().setStretchLastSection(True)
        twLogTable.verticalHeader().setVisible(False)

        twLogTable.setSelectionBehavior(QAbstractItemView.SelectRows)
        twLogTable.setSelectionMode(QAbstractItemView.SingleSelection)

        twLogTable.setSortingEnabled(True)
        twLogTable.sortByColumn(0, Qt.DescendingOrder)

        twLogTable.doubleClicked.connect(self.onItemDoubleClicked)

    def _setupFilterWidget(self):
        """The filter widget and its components is created and returned.
        """
        # Filter options
        filterWidget = QWidget()
        filterWidget.setVisible(False)
        vFilterLayout = QVBoxLayout(filterWidget)
        vFilterLayout.setContentsMargins(0, 0, 0, 0)

        hUpperLayout = QHBoxLayout()
        hUpperLayout.setContentsMargins(0, 0, 0, 0)

        # Search filter
        self.gbSearch = QGroupBox("Search  ")
        self.gbSearch.setFlat(True)

        # Layout for search field
        text = "Search for"
        self.leSearch = QLineEdit()
        self.leSearch.setStatusTip(text)
        self.leSearch.setToolTip(text)
        self.leSearch.textChanged.connect(self.onFilterChanged)
        self.pbSearchInsId = QPushButton("Instance ID")

        hSearchLayout = QHBoxLayout()
        text = "Search instance ID"
        self.pbSearchInsId.setStatusTip(text)
        self.pbSearchInsId.setToolTip(text)
        self.pbSearchInsId.setCheckable(True)
        self.pbSearchInsId.setChecked(False)
        self.pbSearchInsId.clicked.connect(self.onFilterChanged)
        self.pbSearchDescr = QPushButton("Description")
        text = "Search description"
        self.pbSearchDescr.setStatusTip(text)
        self.pbSearchDescr.setToolTip(text)
        self.pbSearchDescr.setCheckable(True)
        self.pbSearchDescr.setChecked(True)
        self.pbSearchDescr.clicked.connect(self.onFilterChanged)
        self.pbSearchAddDescr = QPushButton("Additional description")
        text = "Search additional description"
        self.pbSearchAddDescr.setStatusTip(text)
        self.pbSearchAddDescr.setToolTip(text)
        self.pbSearchAddDescr.setCheckable(True)
        self.pbSearchAddDescr.setChecked(False)
        self.pbSearchAddDescr.clicked.connect(self.onFilterChanged)

        hSearchLayout.addWidget(self.pbSearchInsId)
        hSearchLayout.addWidget(self.pbSearchDescr)
        hSearchLayout.addWidget(self.pbSearchAddDescr)
        hSearchLayout.addStretch()

        vSearchLayout = QVBoxLayout(self.gbSearch)
        vSearchLayout.setContentsMargins(5, 5, 5, 5)
        vSearchLayout.addWidget(self.leSearch)
        vSearchLayout.addLayout(hSearchLayout)

        hUpperLayout.addWidget(self.gbSearch)

        # Date filter
        self.gbDate = QGroupBox("Date filter  ")
        self.gbDate.setFlat(True)
        self.gbDate.setCheckable(True)
        self.gbDate.setChecked(False)
        self.gbDate.clicked.connect(self.onFilterChanged)

        dateLayout = QFormLayout()
        self.dtStartDate = QDateTimeEdit()
        self.dtStartDate.setDisplayFormat("yyyy-MM-dd hh:mm")
        self.dtStartDate.setCalendarPopup(True)
        self.dtStartDate.setDate(QDate.currentDate().addMonths(-1))
        self.dtStartDate.dateTimeChanged.connect(self.onFilterChanged)
        dateLayout.addRow("Start date: ", self.dtStartDate)

        self.dtEndDate = QDateTimeEdit()
        self.dtEndDate.setDisplayFormat("yyyy-MM-dd hh:mm")
        self.dtEndDate.setCalendarPopup(True)
        self.dtEndDate.setDateTime(QDateTime.currentDateTime())
        self.dtEndDate.dateTimeChanged.connect(self.onFilterChanged)
        dateLayout.addRow("End date: ", self.dtEndDate)

        hDateLayout = QHBoxLayout(self.gbDate)
        hDateLayout.setContentsMargins(5, 5, 5, 5)
        hDateLayout.addLayout(dateLayout)
        hDateLayout.addStretch()

        hUpperLayout.addWidget(self.gbDate)

        vFilterLayout.addLayout(hUpperLayout)

        hDateLine = QFrame(self)
        hDateLine.setFrameShape(QFrame.HLine)
        hDateLine.setFrameShadow(QFrame.Sunken)
        vFilterLayout.addWidget(hDateLine)

        # Filter buttons
        hFilterLayout = QHBoxLayout()
        vFilterLayout.addLayout(hFilterLayout)
        hFilterLayout.setSpacing(25)
        self.laFilter = QLabel("Filter message type: ")
        font = self.laFilter.font()
        font.setBold(True)
        self.laFilter.setFont(font)

        text = "Show debug messages"
        self.pbFilterDebug = QToolButton()
        self.pbFilterDebug.setIcon(icons.logDebug)
        self.pbFilterDebug.setMinimumSize(32, 32)
        self.pbFilterDebug.setStatusTip(text)
        self.pbFilterDebug.setToolTip(text)
        self.pbFilterDebug.setCheckable(True)
        self.pbFilterDebug.setChecked(True)
        self.pbFilterDebug.clicked.connect(self.onFilterChanged)

        text = "Show information messages"
        self.pbFilterInfo = QToolButton()
        self.pbFilterInfo.setIcon(icons.logInfo)
        self.pbFilterInfo.setMinimumSize(32, 32)
        self.pbFilterInfo.setStatusTip(text)
        self.pbFilterInfo.setToolTip(text)
        self.pbFilterInfo.setCheckable(True)
        self.pbFilterInfo.setChecked(True)
        self.pbFilterInfo.clicked.connect(self.onFilterChanged)

        text = "Show warn messages"
        self.pbFilterWarn = QToolButton()
        self.pbFilterWarn.setIcon(icons.logWarning)
        self.pbFilterWarn.setMinimumSize(32, 32)
        self.pbFilterWarn.setStatusTip(text)
        self.pbFilterWarn.setToolTip(text)
        self.pbFilterWarn.setCheckable(True)
        self.pbFilterWarn.setChecked(True)
        self.pbFilterWarn.clicked.connect(self.onFilterChanged)

        text = "Show error messages"
        self.pbFilterError = QToolButton()
        self.pbFilterError.setIcon(icons.logError)
        self.pbFilterError.setMinimumSize(32, 32)
        self.pbFilterError.setStatusTip(text)
        self.pbFilterError.setToolTip(text)
        self.pbFilterError.setCheckable(True)
        self.pbFilterError.setChecked(True)
        self.pbFilterError.clicked.connect(self.onFilterChanged)

        hFilterLayout.setContentsMargins(5, 5, 5, 5)
        hFilterLayout.addWidget(self.laFilter)
        hFilterLayout.addWidget(self.pbFilterDebug)
        hFilterLayout.addWidget(self.pbFilterInfo)
        hFilterLayout.addWidget(self.pbFilterWarn)
        hFilterLayout.addWidget(self.pbFilterError)
        hFilterLayout.addStretch()

        return filterWidget

    def onLogDataAvailable(self, logData):
        new = [Log(i, messageType=log["type"], instanceId=log["category"],
                   description=log["message"],
                   additionalDescription=log.get("traceback", ""),
                   dateTime=QDateTime.fromString(log["timestamp"], Qt.ISODate))
               for i, log in enumerate(logData, start=self.tailindex + 1)]
        self.tailindex += len(logData)
        self.logs.extend(new)

        for log in self.filter(new):
            self.queryModel.add(log)
        self.prune()

    def prune(self):
        """delete the oldest 10000 entries if we have more than 100000"""
        if len(self.logs) < 100000:
            return
        self.logs.sort(key=lambda l: l.dateTime, reverse=True)
        self.logs = self.logs[:-10000]
        self.onFilterChanged()

    @pyqtSlot(bool)
    def onFilterOptionVisible(self, checked):
        """This slot is called from here when the filter options should be
        visible or not.
        """
        if checked:
            text = "Hide filter options"
            self.pbFilterOptions.setText("- " + text)
        else:
            text = "Show filter options"
            self.pbFilterOptions.setText("+ " + text)
        self.pbFilterOptions.setStatusTip(text)
        self.pbFilterOptions.setToolTip(text)

        self.filterWidget.setVisible(checked)

    @pyqtSlot()
    def onFilterChanged(self):
        self.queryModel.setList(self.filter(self.logs))

    def filter(self, g):
        """filter relevant items from generator g"""
        buttons = dict(DEBUG=self.pbFilterDebug, INFO=self.pbFilterInfo,
                       WARN=self.pbFilterWarn, ERROR=self.pbFilterError)
        types = {k for k, v in buttons.items() if v.isChecked()}
        if types:
            g = (log for log in g if (log.messageType in types))
            if self.gbDate.isChecked():
                startDateTime = self.dtStartDate.dateTime()
                startTime = startDateTime.time()
                startTime.setHMS(startTime.hour(), startTime.minute(), 0)
                startDateTime.setTime(startTime)
                endDateTime = self.dtEndDate.dateTime()
                endTime = endDateTime.time()
                endTime.setHMS(endTime.hour(), endTime.minute(), 59)
                endDateTime.setTime(endTime)

                # Check start and end range
                if endDateTime < startDateTime:
                    self.dtStartDate.setDateTime(endDateTime)
                    self.dtEndDate.setDateTime(startDateTime)

                g = (log for log in g
                     if startDateTime < log.dateTime < endDateTime)

            text = self.leSearch.text()
            if text:
                ins = self.pbSearchInsId.isChecked()
                des = self.pbSearchDescr.isChecked()
                add = self.pbSearchAddDescr.isChecked()
                g = (log for log in g
                     if (ins and text in log.instanceId) or
                     (des and text in log.description) or
                     (add and text in log.additionalDescription))

            return list(g)

        return []

    @pyqtSlot(QModelIndex)
    def onItemDoubleClicked(self, index):
        value = index.data()
        if value is None:
            return
        index = self.queryModel.index(index.row(), DEVICE_COLUMN)
        deviceId = self.queryModel.data(index, Qt.DisplayRole)
        broadcast_event(KaraboEvent.ShowDevice, {'deviceId': deviceId})

    @pyqtSlot()
    def onSaveToFile(self):
        """Write current database content to a file """
        filename = getSaveFileName(
            caption="Save file as",
            filter="Log files (*.log)",
            suffix="log",
            parent=self)
        if not filename:
            return

        with open(filename, "w") as out:
            for log in self.logs:
                out.write("{0} | {1} | {2} | {3} | {4} | {5}#\n".format(*log))

    @pyqtSlot()
    def onClearLog(self):
        self.logs = []
        self.onFilterChanged()


Log = namedtuple('Log', ["id", "dateTime", "messageType", "instanceId",
                         "description", "additionalDescription"])


class LogQueryModel(QAbstractTableModel):
    def __init__(self, parent=None):
        super(LogQueryModel, self).__init__(parent)
        self.filtered = []
        self.key = lambda l: l[0]
        self.reverse = False

    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return ("ID", "Date and time", "Message type", "Instance ID",
                    "Description", "Additional description")[section]

    def setList(self, list_):
        self.beginResetModel()
        self.filtered = list_
        self.filtered.sort(key=self.key, reverse=self.reverse)
        self.endResetModel()

    def rowCount(self, _):
        return len(self.filtered)

    def columnCount(self, _):
        return 6

    icons = dict(DEBUG=icons.logDebug, INFO=icons.logInfo,
                 WARN=icons.logWarning, WARN_LOW=icons.logWarning,
                 WARN_HIGH=icons.logWarning, ERROR=icons.logError,
                 ALARM_LOW=icons.logAlarm, ALARM_HIGH=icons.logAlarm)

    textColor = dict(DEBUG=QColor(0, 128, 0), INFO=QColor(0, 0, 128),
                     WARN=QColor(255, 102, 0), WARN_LOW=QColor(255, 102, 0),
                     WARN_HIGH=QColor(255, 102, 0),
                     ALARM_LOW=QColor(255, 204, 102),
                     ALARM_HIGH=QColor(255, 204, 102), ERROR=QColor(128, 0, 0))

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None

        log = self.filtered[index.row()]
        if role == Qt.DecorationRole and index.column() == 2:
            return self.icons.get(log.messageType)
        elif role == Qt.TextColorRole and index.column() == 2:
            return self.textColor.get(log.messageType)
        elif role in (Qt.DisplayRole, Qt.ToolTipRole):
            return log[index.column()]
        return None

    def add(self, data):
        hi = len(self.filtered)
        lo = 0
        key = self.key(data)

        while hi > lo:
            mid = (hi + lo) // 2
            if self.reverse == (self.key(self.filtered[mid]) < key):
                hi = mid
            else:
                lo = mid + 1
        self.beginInsertRows(QModelIndex(), lo, lo)
        self.filtered.insert(lo, data)
        self.endInsertRows()

    def sort(self, column, order):
        self.key = lambda l: l[column]
        self.reverse = order != Qt.AscendingOrder
        self.beginResetModel()
        self.filtered.sort(key=self.key, reverse=self.reverse)
        self.endResetModel()
