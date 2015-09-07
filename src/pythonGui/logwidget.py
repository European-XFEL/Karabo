#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 4, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget which shows log messages
   like Debug, Info, Errors, Warns, Alarms, Warnings in a generic kind of way.
"""

__all__ = ["LogWidget", "LogTableView", "LogQueryModel", "LogThread"]


import globals
import icons
from manager import Manager
from util import getSaveFileName

from PyQt4.QtCore import (pyqtSignal, QAbstractTableModel, QDate, QDateTime, Qt)
from PyQt4.QtGui import (QAbstractItemView, QColor, QDateTimeEdit,
                         QFormLayout, QFrame, QGroupBox, QHBoxLayout,
                         QItemSelectionModel, QLabel,
                         QLineEdit, QPushButton, QTableView, QToolButton,
                         QVBoxLayout, QWidget)

from collections import namedtuple


class LogWidget(QWidget):


    def __init__(self, parent=None, isLogData=True):
        # parent - parent widget
        # isLogData - describes whether this widget is a normal log or an notification widget
        super(LogWidget, self).__init__(parent)

        self.isLogData = isLogData

        # Main layout
        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(0,0,0,0)

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

        self.logs = [ ]
        self.queryModel = LogQueryModel()
        # Create thread for log data processing
        self.twLogTable = LogTableView(self.queryModel, self)
        vLayout.addWidget(self.twLogTable)


    def _setupFilterWidget(self):
        """
        The filter widget and its components is created and returned.
        """

        # Filter options
        filterWidget = QWidget()
        filterWidget.setVisible(False)
        vFilterLayout = QVBoxLayout(filterWidget)
        vFilterLayout.setContentsMargins(0,0,0,0)

        hUpperLayout = QHBoxLayout()
        hUpperLayout.setContentsMargins(0,0,0,0)

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
        vSearchLayout.setContentsMargins(5,5,5,5)
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
        self.dtStartDate.setDate(QDate(2014, 1, 1))
        self.dtStartDate.dateTimeChanged.connect(self.onFilterChanged)
        dateLayout.addRow("Start date: ", self.dtStartDate)

        self.dtEndDate = QDateTimeEdit()
        self.dtEndDate.setDisplayFormat("yyyy-MM-dd hh:mm")
        self.dtEndDate.setCalendarPopup(True)
        self.dtEndDate.setDateTime(QDateTime.currentDateTime())
        self.dtEndDate.dateTimeChanged.connect(self.onFilterChanged)
        dateLayout.addRow("End date: ", self.dtEndDate)

        hDateLayout = QHBoxLayout(self.gbDate)
        hDateLayout.setContentsMargins(5,5,5,5)
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
        self.pbFilterDebug.setMinimumSize(32,32)
        self.pbFilterDebug.setStatusTip(text)
        self.pbFilterDebug.setToolTip(text)
        self.pbFilterDebug.setCheckable(True)
        self.pbFilterDebug.setChecked(True)
        self.pbFilterDebug.clicked.connect(self.onFilterChanged)

        text = "Show information messages"
        self.pbFilterInfo = QToolButton()
        self.pbFilterInfo.setIcon(icons.logInfo)
        self.pbFilterInfo.setMinimumSize(32,32)
        self.pbFilterInfo.setStatusTip(text)
        self.pbFilterInfo.setToolTip(text)
        self.pbFilterInfo.setCheckable(True)
        self.pbFilterInfo.setChecked(True)
        self.pbFilterInfo.clicked.connect(self.onFilterChanged)

        text = "Show warn messages"
        self.pbFilterWarn = QToolButton()
        self.pbFilterWarn.setIcon(icons.logWarning)
        self.pbFilterWarn.setMinimumSize(32,32)
        self.pbFilterWarn.setStatusTip(text)
        self.pbFilterWarn.setToolTip(text)
        self.pbFilterWarn.setCheckable(True)
        self.pbFilterWarn.setChecked(True)
        self.pbFilterWarn.clicked.connect(self.onFilterChanged)

        text = "Show error messages"
        self.pbFilterError = QToolButton()
        self.pbFilterError.setIcon(icons.logError)
        self.pbFilterError.setMinimumSize(32,32)
        self.pbFilterError.setStatusTip(text)
        self.pbFilterError.setToolTip(text)
        self.pbFilterError.setCheckable(True)
        self.pbFilterError.setChecked(True)
        self.pbFilterError.clicked.connect(self.onFilterChanged)

        text = "Show alarm messages"
        self.pbFilterAlarm = QToolButton()
        self.pbFilterAlarm.setIcon(icons.logAlarm)
        self.pbFilterAlarm.setMinimumSize(32,32)
        self.pbFilterAlarm.setStatusTip(text)
        self.pbFilterAlarm.setToolTip(text)
        self.pbFilterAlarm.setCheckable(True)
        self.pbFilterAlarm.setChecked(True)
        self.pbFilterAlarm.clicked.connect(self.onFilterChanged)

        text = "Show warning messages"
        self.pbFilterWarning = QToolButton()
        self.pbFilterWarning.setIcon(icons.logWarning)
        self.pbFilterWarning.setMinimumSize(32,32)
        self.pbFilterWarning.setStatusTip(text)
        self.pbFilterWarning.setToolTip(text)
        self.pbFilterWarning.setCheckable(True)
        self.pbFilterWarning.setChecked(True)
        self.pbFilterWarning.clicked.connect(self.onFilterChanged)

        hFilterLayout.setContentsMargins(5,5,5,5)
        hFilterLayout.addWidget(self.laFilter)
        hFilterLayout.addWidget(self.pbFilterDebug)
        hFilterLayout.addWidget(self.pbFilterInfo)
        hFilterLayout.addWidget(self.pbFilterWarn)
        hFilterLayout.addWidget(self.pbFilterError)
        hFilterLayout.addWidget(self.pbFilterAlarm)
        hFilterLayout.addWidget(self.pbFilterWarning)
        hFilterLayout.addStretch()

        if self.isLogData:
            # Show these buttons
            self.pbFilterDebug.setChecked(True)
            self.pbFilterDebug.setVisible(True)
            self.pbFilterInfo.setChecked(True)
            self.pbFilterInfo.setVisible(True)
            self.pbFilterWarn.setChecked(True)
            self.pbFilterWarn.setVisible(True)
            self.pbFilterError.setChecked(True)
            self.pbFilterError.setVisible(True)
            # Do not show these buttons
            self.pbFilterAlarm.setChecked(True)
            self.pbFilterAlarm.setVisible(False)
            self.pbFilterWarning.setChecked(True)
            self.pbFilterWarning.setVisible(False)
        else:
            # Do not show these buttons
            self.pbFilterDebug.setChecked(True)
            self.pbFilterDebug.setVisible(False)
            self.pbFilterInfo.setChecked(True)
            self.pbFilterInfo.setVisible(False)
            self.pbFilterWarn.setChecked(True)
            self.pbFilterWarn.setVisible(False)
            # Show these buttons
            self.pbFilterError.setChecked(True)
            self.pbFilterError.setVisible(True)
            self.pbFilterAlarm.setChecked(True)
            self.pbFilterAlarm.setVisible(True)
            self.pbFilterWarning.setChecked(True)
            self.pbFilterWarning.setVisible(True)

        return filterWidget


    def onLogDataAvailable(self, logData):
        for l in logData:
            self.logs.append(Log(
                len(self.logs) + 1, dateTime=l["timestamp"],
                messageType=l["type"], instanceId=l["category"],
                description=l["message"], additionalDescription=""))
        self.onViewNeedsUpdate()


    def onViewNeedsUpdate(self):
        # Update view considering filter options
        self.onFilterChanged()


    def onFilterOptionVisible(self, checked):
        """
        This slot is called from here when the filter options should be visible
        or not.
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


    def onFilterChanged(self):
        """
        This slot is called from here when the filter options might have changed
        and the view needs to be updated by a new query to the database.
        """
        g = self.logs
        if self.isLogData:
            s = dict(DEBUG=self.pbFilterDebug, INFO=self.pbFilterInfo,
                     WARN=self.pbFilterWarn, ERROR=self.pbFilterError)
        else:
            s = dict(ERROR=self.pbFilterError, ALARM_LOW=self.pbFilterAlarm,
                     ALARM_HIGH=self.pbFilterAlarm,
                     WARN_LOW=self.pbFilterWarning,
                     WARN_HIGH=self.pbFilterWarning)
        msgs = {k for k, v in s.items() if v.isChecked()}
        if msgs:
            g = (l for l in g if (l.messageType in msgs))

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

            g = (l for l in g if startDateTime <
                 QDateTime.fromString(l.dateTime, Qt.ISODate) <
                 endDateTime)

        text = self.leSearch.text()
        if text:
            ins = self.pbSearchInsId.isChecked()
            des = self.pbSearchDescr.isChecked()
            add = self.pbSearchAddDescr.isChecked()
            g = (l for l in g if (ins and text in l.instanceId) or
                                 (des and text in l.description) or
                                 (add and text in l.additionalDescription))
        self.queryModel.setList(list(g))


    def onSaveToFile(self):
        """ Write current database content to a file """
        filename = getSaveFileName(
            "Save file as",globals.HIDDEN_KARABO_FOLDER,
            "Log files (*.log)", "log")
        if not filename:
            return

        with open(filename, "w") as out:
            for l in self.logs:
                out.write("{0} | {1} | {2} | {3} | {4} | {5}#\n".format(*l))


    def onClearLog(self):
        self.logs = [ ]
        self.onFilterChanged()


class LogTableView(QTableView):
    def __init__(self, model, parent=None):
        super(LogTableView, self).__init__(parent)

        # Model
        self.setModel(model)

        self.setWordWrap(True)
        self.setAlternatingRowColors(True)
        self.horizontalHeader().setStretchLastSection(True)
        self.verticalHeader().setVisible(False)

        # Selection
        self.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.setSelectionMode(QAbstractItemView.SingleSelection)

        # Sorting
        self.setSortingEnabled(True)
        self.sortByColumn(0, Qt.AscendingOrder)


    def mouseDoubleClickEvent(self, event):
        index = self.model().index(self.currentIndex().row(), 3)
        value = index.data()
        if value is None:
            return
        # Emit signal with deviceId to select device instance
        Manager().signalSelectNewNavigationItem.emit(value)
        QTableView.mouseDoubleClickEvent(self, event)


Log = namedtuple('Log', ["id", "dateTime", "messageType", "instanceId",
                         "description", "additionalDescription"])


class LogQueryModel(QAbstractTableModel):
    # Define signals
    signalViewNeedsSortUpdate = pyqtSignal(str) # queryText
    signalRestoreLastSelection = pyqtSignal(object) # modelIndex

    def __init__(self, parent=None):
        super(LogQueryModel, self).__init__(parent)

        self.filtered = [ ]


    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return ("ID", "Date and time", "Message type", "Instance ID",
                    "Description", "Additional description")[section]

    def setList(self, l):
        self.beginResetModel()
        self.filtered = l
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
                     ALARM_HIGH=QColor(255, 204, 102), ERROR=QColor(128,0,0))

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None

        l = self.filtered[index.row()]
        if role == Qt.DecorationRole and index.column() == 2:
            return self.icons.get(l.messageType)
        elif role == Qt.TextColorRole and index.column() == 2:
            return self.textColor.get(l.messageType)
        elif role in (Qt.DisplayRole, Qt.ToolTipRole):
            return l[index.column()]
        return None

    def sort(self, column, order):
        self.beginResetModel()
        self.filtered.sort(key=lambda l: l[column],
                           reverse=order != Qt.AscendingOrder)
        self.endResetModel()
