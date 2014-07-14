#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 4, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget which shows log messages
   like Debug, Info, Errors, Warns, Alarms, Warnings in a generic kind of way.
"""

__all__ = ["LogWidget", "LogTableView", "LogSqlQueryModel", "LogThread"]


import globals
import icons
from manager import Manager
from util import getSaveFileName

from PyQt4.QtCore import (pyqtSignal, QDate, QDateTime, QIODevice, QMutex,
                          QMutexLocker, Qt, QThread)
from PyQt4.QtGui import (QAbstractItemView, QColor, QDateTimeEdit,
                         QFormLayout, QFrame, QGroupBox, QHBoxLayout,
                         QHeaderView, QLabel,
                         QLineEdit, QPushButton, QTableView, QToolButton,
                         QVBoxLayout, QWidget)

from Queue import Queue
from time import sleep

try:
    from PyQt4.QtSql import QSqlTableModel, QSqlQuery, QSqlQueryModel
except:
    print "*ERROR* The PyQt4 sql module is not installed"


# Define date time format
dateTimeFormat = "yyyy-MM-dd hh:mm:ss"


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
        self.pbFilterOptions = QPushButton("+ " + text)
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

        # Create mutexes
        self.logDataMutex = QMutex()
        self.modelMutex = QMutex()

        # Create mutexes
        self.logDataMutex = QMutex()
        self.modelMutex = QMutex()

        # Create sql query model
        self.sqlQueryModel = LogSqlQueryModel()

        # Concats arriving log messages
        self.logDataQueue = Queue()
        
        # Create thread for log data processing
        self.logThread = LogThread(self, self.logDataQueue, self.logDataMutex)
        self.logThread.signalViewNeedsUpdate.connect(self.onViewNeedsUpdate)
        self.logThread.start()

        # Create thread for log data processing
        self.twLogTable = LogTableView(self.sqlQueryModel, self)
        vLayout.addWidget(self.twLogTable)

        # Current view state
        self.viewState = self.twLogTable.horizontalHeader().saveState()
        
        # Connect signal
        self.sqlQueryModel.signalViewNeedsSortUpdate.connect(self.onViewNeedsSortUpdate)


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


    def addNotificationMessage(self, notificationData):
        with QMutexLocker(self.logDataMutex):
            self.logDataQueue.put(notificationData)


### slots ###
    def onLogDataAvailable(self, logData):
        """
        This slot is called from the LoggingPanel when new logging data is
        available.
        """
        with QMutexLocker(self.logDataMutex):
            self.logDataQueue.put(logData)


    def onViewNeedsUpdate(self):
        # Update view considering filter options
        self.onFilterChanged()


    def onViewNeedsSortUpdate(self, queryText):
        """
        This slot is called from SqlQueryModel whenever the sorting changed and
        the previous query needs to be called but with the new sorting query
        added.
        """
        with QMutexLocker(self.modelMutex):
            self.sqlQueryModel.setLogQuery(queryText)

        self.twLogTable.resizeRowsToContents()


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
        # Save current view state
        self.viewState = self.twLogTable.horizontalHeader().saveState()

        filterQuery = ""

        msgTypeFilter = ""
        filterApplied = False
        if self.isLogData:
            if self.pbFilterDebug.isChecked():
                if filterApplied:
                    msgTypeFilter += " OR "
                msgTypeFilter += "messageType='DEBUG'"
                filterApplied = True
            if self.pbFilterInfo.isChecked():
                if filterApplied:
                    msgTypeFilter += " OR "
                msgTypeFilter += "messageType='INFO'"
                filterApplied = True
            if self.pbFilterWarn.isChecked():
                if filterApplied:
                    msgTypeFilter += " OR "
                msgTypeFilter += "messageType='WARN'"
                filterApplied = True
            if self.pbFilterError.isChecked():
                if filterApplied:
                    msgTypeFilter += " OR "
                msgTypeFilter += "messageType='ERROR'"
                filterApplied = True
        else:
            if self.pbFilterError.isChecked():
                if filterApplied:
                    msgTypeFilter += " OR "
                msgTypeFilter += "messageType='ERROR'"
                filterApplied = True
            if self.pbFilterAlarm.isChecked():
                if filterApplied:
                    msgTypeFilter += " OR "
                msgTypeFilter += "messageType='ALARM_LOW' OR messageType='ALARM_HIGH'"
                filterApplied = True
            if self.pbFilterWarning.isChecked():
                if filterApplied:
                    msgTypeFilter += " OR "
                msgTypeFilter += "messageType='WARN_LOW' OR messageType='WARN_HIGH'"
                filterApplied = True

        if len(msgTypeFilter) > 0:
            filterQuery += "(" +msgTypeFilter+ ")"
        else:
            filterQuery += "NOT EXISTS (messageType='DEBUG' AND messageType='INFO' " \
                                   "AND messageType='WARN' AND messageType='ERROR' " \
                                   "AND messageType='ALARM_LOW' AND messageType='ALARM_HIGH' " \
                                   "AND messageType='WARN_LOW' AND messageType='WARN_HIGH')"
            filterApplied = True

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
                return

            if filterApplied:
                filterQuery += " AND "
            filterQuery += "(dateTime >= strftime('%Y-%m-%d %H:%M:%S', '" +startDateTime.toString(dateTimeFormat)+ "') AND " \
                            "dateTime <= strftime('%Y-%m-%d %H:%M:%S', '" +endDateTime.toString(dateTimeFormat)+ "')" \
                           ")"
            filterApplied = True

        # Text search options
        searchText = self.leSearch.text()
        searchQuery = ""
        if len(searchText) > 0:
            if self.pbSearchInsId.isChecked():
                if len(searchQuery) > 0:
                    searchQuery += " OR "
                searchQuery += "instanceId LIKE '%" +searchText+ "%'"
                filterApplied = True
            if self.pbSearchDescr.isChecked():
                if len(searchQuery) > 0:
                    searchQuery += " OR "
                searchQuery += "description LIKE '%" +searchText+ "%'"
                filterApplied = True
            if self.pbSearchAddDescr.isChecked():
                if len(searchQuery) > 0:
                    searchQuery += " OR "
                searchQuery += "additionalDescription LIKE '%" +searchText+ "%'"
                filterApplied = True
            # Put it all together
            if filterApplied and len(searchQuery) > 0:
                filterQuery += " AND (" + searchQuery + ")"

        queryText = "SELECT id, dateTime, messageType, instanceId, description, additionalDescription FROM tLog"
        if filterApplied:
            queryText += " WHERE "
            queryText += filterQuery

        with QMutexLocker(self.modelMutex):
            self.sqlQueryModel.setLogQuery(queryText)

        # Restore current view state
        self.twLogTable.horizontalHeader().restoreState(self.viewState)
        # Resize row contents
        self.twLogTable.resizeRowsToContents()


    def onSaveToFile(self):
        """ Write current database content to a file """
        filename = getSaveFileName(
            "Save file as",globals.HIDDEN_KARABO_FOLDER,
            "Log files (*.log)", "log")
        if not filename:
            return

        with open(filename, "w") as out:
            model = QSqlQueryModel()
            queryText = """SELECT id, dateTime, messageType, instanceId,
                               description, additionalDescription
                           FROM tLog ORDER BY dateTime DESC;"""
            model.setQuery(queryText, Manager().sqlDatabase)

            for i in xrange(model.rowCount()):
                id = model.record(i).value("id")
                dateTime = model.record(i).value("dateTime")
                messageType = model.record(i).value("messageType")
                instanceId = model.record(i).value("instanceId")
                description = model.record(i).value("description")
                additionalDescription = model.record(i).value(
                    "additionalDescription")

                out.write("{} | {} | {} | {} | {} | {}#\n".
                          format(id, dateTime, messageType, instanceId,
                                 description, additionalDescription))


    def onClearLog(self):
        # Remove all message from database
        queryText = "DELETE FROM tLog;"
        model = QSqlQueryModel()
        model.setQuery(queryText, Manager().sqlDatabase)

        # Reset last selected id
        with QMutexLocker(self.modelMutex):
            self.sqlQueryModel.lastSelectedId = None

        # Update
        self.onFilterChanged()



class LogTableView(QTableView):


    def __init__(self, model, parent=None):
        super(LogTableView, self).__init__(parent)

        # Model
        self.setModel(model)
        model.signalRestoreLastSelection.connect(self.onRestoreLastSelection)
        
        # Selection
        self.selectionModel().selectionChanged.connect(self.onSelectionChanged)
        self.lastSelectedId = None

        self.setWordWrap(True)
        self.setAlternatingRowColors(True)
        self.horizontalHeader().setStretchLastSection(True)
        #self.horizontalHeader().setResizeMode(QHeaderView.ResizeToContents)
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


    def onRestoreLastSelection(self, index):
        # Select index
        self.selectionModel().blockSignals(True)
        self.selectionModel().select(index,
                   QItemSelectionModel.Rows | QItemSelectionModel.SelectCurrent)
        self.selectionModel().blockSignals(False)
        
        # Scroll to selected index
        #self.blockSignals(True)
        #self.scrollTo(index, QAbstractItemView.PositionAtCenter)
        #self.blockSignals(False)


    def onSelectionChanged(self, selected, deselected):
        indexes = selected.indexes()
        nbIndexes = len(indexes)
        if nbIndexes < 1:
            return
        # Save database unique ID
        self.model().lastSelectedId = indexes[0].data(Qt.DisplayRole)



class LogSqlQueryModel(QSqlQueryModel):
    # Define signals
    signalViewNeedsSortUpdate = pyqtSignal(str) # queryText
    signalRestoreLastSelection = pyqtSignal(object) # modelIndex

    def __init__(self, parent=None, preQueryText="", sortByColumn=0,
                 sortOrder=Qt.AscendingOrder):
        super(LogSqlQueryModel, self).__init__(parent)

        self.preQueryText = preQueryText
        self.sortByColumn = sortByColumn
        self.sortOrder = sortOrder
        self.lastSelectedId = None


    def setLogQuery(self, queryText=""):
        if len(queryText) == 0:
            queryText = "SELECT id, dateTime, messageType, instanceId, description, additionalDescription FROM tLog;"

        self.preQueryText = queryText

        # Consider sorting
        if self.sortByColumn == 0:
            sortBy = "id"
        elif self.sortByColumn == 1:
            sortBy = "dateTime"
        elif self.sortByColumn == 2:
            sortBy = "messageType"
        elif self.sortByColumn == 3:
            sortBy = "instanceId"
        elif self.sortByColumn == 4:
            sortBy = "description"
        elif self.sortByColumn == 5:
            sortBy = "additionalDescription"

        if self.sortOrder == Qt.AscendingOrder:
            order = "ASC"
        else:
            order = "DESC"
        queryText += " ORDER BY " + sortBy + " " + order
        
        self.setQuery(queryText)

        self.setHeaderData(0, Qt.Horizontal, "ID")
        self.setHeaderData(1, Qt.Horizontal, "Date and time")
        self.setHeaderData(2, Qt.Horizontal, "Message type")
        self.setHeaderData(3, Qt.Horizontal, "Instance ID")
        self.setHeaderData(4, Qt.Horizontal, "Description")
        self.setHeaderData(5, Qt.Horizontal, "Additional description")

	#while self.canFetchMore():
        #    self.fetchMore()


    icons = dict(DEBUG=icons.logDebug, INFO=icons.logInfo,
                 WARN=icons.logWarning, WARN_LOW=icons.logWarning,
                 WARN_HIGH=icons.logWarning, ERROR=icons.logError,
                 ALARM_LOW=icons.logAlarm, ALARM_HIGH=icons.logAlarm)


    def getTextColor(self, value):
        if value == 'DEBUG':
            return QColor(0,128,0)
        elif value == 'INFO':
            return QColor(0,0,128)
        elif (value == 'WARN') or (value == 'WARN_LOW') or (value == 'WARN_HIGH'):
            return QColor(255,102,0)
        elif (value == 'ALARM_LOW') or (value == 'ALARM_HIGH'):
            return QColor(255,204,102)
        elif value == 'ERROR':
            return QColor(128,0,0)
        return None


    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None

        if role == Qt.DecorationRole and index.column() == 2:
            # Get text for comparison to get correct icon
            modelIndex = QSqlQueryModel.index(self, index.row(), index.column())
            return self.icons.get(modelIndex.data(Qt.DisplayRole)).icon
        elif role == Qt.TextColorRole and index.column() == 2:
            # Get text for comparison to get correct text color
            modelIndex = QSqlQueryModel.index(self, index.row(), index.column())
            return self.getTextColor(modelIndex.data(Qt.DisplayRole))
        elif role == Qt.DisplayRole:
            value = QSqlQueryModel.data(self, index, role)
            if (index.column() == 0) and (value == self.lastSelectedId):
                self.signalRestoreLastSelection.emit(index)
            return value
        elif role == Qt.ToolTipRole:
            modelIndex = QSqlQueryModel.index(self, index.row(), index.column())
            return modelIndex.data(Qt.DisplayRole)
        return None


    def sort(self, column, order):
        self.sortByColumn = column
        self.sortOrder = order
        # Send signal to update view
        self.signalViewNeedsSortUpdate.emit(self.preQueryText)


class LogThread(QThread):
    # Define signals
    signalViewNeedsUpdate = pyqtSignal()

    def __init__(self, parent, logDataQueue, logDataMutex):
        super(LogThread, self).__init__(parent)

        self.sqlQueryModel = None

        self.modelMutex = QMutex()

        self.logDataQueue = logDataQueue
        self.logDataMutex = logDataMutex

        self.isFinished = False
        # Delete when no longer needed
        self.finished.connect(self.onDeleteLater)


    def getLogDataBlock(self):
        # Put all data from current queue into string
        logBlock = ""
        with QMutexLocker(self.logDataMutex):
            while not self.logDataQueue.empty():
                logBlock += self.logDataQueue.get()
        return logBlock


    def processLogData(self, logData):
        # logData needs to be split up
        if len(logData) < 1:
            return

        logList = logData.split('#')
        for logMsg in logList:
            if len(logMsg) < 1:
                continue

            logMsgList = logMsg.split(' | ')
            if len(logMsgList) < 1:
                continue

            dateTime = logMsgList[0]
            logLevel = logMsgList[1]
            instanceId = logMsgList[2]
            description = logMsgList[3]
            if len(logMsgList) > 4:
                additionalDescription = logMsgList[4]
            else:
                additionalDescription = ""
            self.insertInto(dateTime, logLevel, instanceId, description, additionalDescription)

        # Performance test
        #i = 20
        #while i > 0:
        #    self.insertInto(QDateTime.currentDateTime().toString(dateTimeFormat), "INFO", "pcx17673/DemoDevice/100", "This is short.", "LOW")
        #    i -= 1


    def insertInto(self, dateTime, msgType, instanceId, description, additionalDescription=""):
        # Check number of rows in database
        queryText = "SELECT count(1) FROM tLog;"
        query = QSqlQuery(queryText, Manager().sqlDatabase)
        nbRows = 0
        while query.next():
            nbRows = query.value(0)

        # Remove rows if limit is reached
        rowLimit = 20000
        if nbRows > rowLimit:
            queryText = "DELETE FROM tLog WHERE id in (  \
                      SELECT t.id FROM tLog t ORDER BY t.dateTime asc limit 10);"
            with QMutexLocker(self.modelMutex):
                self.sqlQueryModel.setQuery(queryText)

        # Insert parameter into database
        queryText = "INSERT INTO tLog (dateTime, messageType, instanceId, description, additionalDescription) " \
                    "VALUES (strftime('%Y-%m-%d %H:%M:%S','" + dateTime + "'), '" +msgType+ "', '" +instanceId+ \
                    "', '" +description+ "', '" +additionalDescription+ "');"
        with QMutexLocker(self.modelMutex):
            self.sqlQueryModel.setQuery(queryText)


    def run(self):
        while not self.isFinished:
            if not self.logDataQueue.empty():

                # Create new model for tableView, otherwise update gets messy/blank for a while
                if self.sqlQueryModel:
                    preQueryText = self.sqlQueryModel.preQueryText
                    sortByColumn = self.sqlQueryModel.sortByColumn
                    sortOrder = self.sqlQueryModel.sortOrder
                    self.sqlQueryModel = LogSqlQueryModel(None, preQueryText, sortByColumn, sortOrder)
                else:
                    self.sqlQueryModel = LogSqlQueryModel()

                # Get log block of current queue
                logBlock = self.getLogDataBlock()
                # Insert log block into database
                self.processLogData(logBlock)
                # Notify main thread
                self.signalViewNeedsUpdate.emit()
            # Sleep for 1 sec
            sleep(1)


    def onDeleteLater(self):
        print "onDeleteLater"
        self.isFinished = True
        self.wait()

