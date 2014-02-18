#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 4, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget which shows log messages
   like Debug, Info, Errors, Warns, Alarms, Warnings in a generic kind of way.
"""

__all__ = ["LogWidget", "LogTableView", "LogSqlQueryModel", "LogThread"]


from manager import Manager

from PyQt4.QtCore import (pyqtSignal, QDate, QDateTime, QMutex, QMutexLocker,
                          Qt, QThread)
from PyQt4.QtGui import (QAbstractItemView, QColor, QDateTimeEdit, QFormLayout,
                         QFrame, QGroupBox, QHBoxLayout, QHeaderView, QIcon,
                         QItemSelection, QItemSelectionModel, QLabel, QLineEdit,
                         QPushButton, QTableView, QToolButton, QVBoxLayout,
                         QWidget)

from Queue import Queue
from time import sleep

try:
    from PyQt4.QtSql import QSqlTableModel, QSqlQueryModel
except:
    print "*ERROR* The PyQt4 sql module is not installed"


# Define date time format
dateTimeFormat = "yyyy-MM-dd hh:mm:ss"

# Define some useful dictionaries for fast lookup
#LOG_LEVEL = {'DEBUG':0, 'INFO':1, 'WARN':2, 'ERROR':3, 'WARNING':4, 'ALARM':5}
#LOG_TEXT = {0:'DEBUG', 1:'INFO', 2:'WARN', 3:'ERROR', 4:'WARNING', 5:'ALARM'}


class LogWidget(QWidget):


    def __init__(self, parent=None, isLogData=True):
        # parent - parent widget
        # isLogData - describes whether this widget is a normal log or an notification widget
        super(LogWidget, self).__init__(parent)

        self.__isLogData = isLogData

        # Main layout
        vLayout = QVBoxLayout(self)
        vLayout.setContentsMargins(0,0,0,0)

        # Add button to collapse/expand filter options
        text = "Show filter options"
        self.__pbFilterOptions = QPushButton("+ " + text)
        self.__pbFilterOptions.setStatusTip(text)
        self.__pbFilterOptions.setToolTip(text)
        self.__pbFilterOptions.setCheckable(True)
        self.__pbFilterOptions.setChecked(False)

        font = self.__pbFilterOptions.font()
        font.setBold(True)
        self.__pbFilterOptions.setFont(font)
        self.__pbFilterOptions.clicked.connect(self.onFilterOptionVisible)
        vLayout.addWidget(self.__pbFilterOptions)

        # Create widget with filter options
        self.__filterWidget = self._setupFilterWidget()
        vLayout.addWidget(self.__filterWidget)

        # Create mutexes
        self.__logDataMutex = QMutex()
        self.__modelMutex = QMutex()

        # Create sql query model
        self.sqlQueryModel = LogSqlQueryModel()
        #self.sqlQueryModel.signalViewNeedsSortUpdate.connect(self.onViewNeedsSortUpdate)

        # Concats arriving log messages
        self.__logDataQueue = Queue()
        # Create thread for log data processing
        self.__logThread = LogThread(self, self.sqlQueryModel, self.__logDataQueue, self.__logDataMutex)
        self.__logThread.signalViewNeedsUpdate.connect(self.onViewNeedsUpdate)
        self.__logThread.start()

        # Create thread for log data processing
        self.__twLogTable = LogTableView(self.sqlQueryModel, self)
        vLayout.addWidget(self.__twLogTable)

        # Current view state
        self.viewState = self.__twLogTable.horizontalHeader().saveState()


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
        self.__gbSearch = QGroupBox("Search  ")
        self.__gbSearch.setFlat(True)

        # Layout for search field
        text = "Search for"
        self.__leSearch = QLineEdit()
        self.__leSearch.setStatusTip(text)
        self.__leSearch.setToolTip(text)
        #self.__leSearch.textChanged.connect(self.onFilterChanged)
        self.__pbSearchInsId = QPushButton("Instance ID")

        hSearchLayout = QHBoxLayout()
        text = "Search instance ID"
        self.__pbSearchInsId.setStatusTip(text)
        self.__pbSearchInsId.setToolTip(text)
        self.__pbSearchInsId.setCheckable(True)
        self.__pbSearchInsId.setChecked(False)
        #self.__pbSearchInsId.clicked.connect(self.onFilterChanged)
        self.__pbSearchDescr = QPushButton("Description")
        text = "Search description"
        self.__pbSearchDescr.setStatusTip(text)
        self.__pbSearchDescr.setToolTip(text)
        self.__pbSearchDescr.setCheckable(True)
        self.__pbSearchDescr.setChecked(True)
        #self.__pbSearchDescr.clicked.connect(self.onFilterChanged)
        self.__pbSearchAddDescr = QPushButton("Additional description")
        text = "Search additional description"
        self.__pbSearchAddDescr.setStatusTip(text)
        self.__pbSearchAddDescr.setToolTip(text)
        self.__pbSearchAddDescr.setCheckable(True)
        self.__pbSearchAddDescr.setChecked(False)
        #self.__pbSearchAddDescr.clicked.connect(self.onFilterChanged)

        hSearchLayout.addWidget(self.__pbSearchInsId)
        hSearchLayout.addWidget(self.__pbSearchDescr)
        hSearchLayout.addWidget(self.__pbSearchAddDescr)
        hSearchLayout.addStretch()

        vSearchLayout = QVBoxLayout(self.__gbSearch)
        vSearchLayout.setContentsMargins(5,5,5,5)
        vSearchLayout.addWidget(self.__leSearch)
        vSearchLayout.addLayout(hSearchLayout)

        hUpperLayout.addWidget(self.__gbSearch)

        # Date filter
        self.__gbDate = QGroupBox("Date filter  ")
        self.__gbDate.setFlat(True)
        self.__gbDate.setCheckable(True)
        self.__gbDate.setChecked(False)
        #self.__gbDate.clicked.connect(self.onFilterChanged)

        dateLayout = QFormLayout()
        self.__dtStartDate = QDateTimeEdit()
        self.__dtStartDate.setDisplayFormat("yyyy-MM-dd hh:mm")
        self.__dtStartDate.setCalendarPopup(True)
        self.__dtStartDate.setDate(QDate(2012, 1, 1))
        #self.__dtStartDate.dateTimeChanged.connect(self.onFilterChanged)
        dateLayout.addRow("Start date: ", self.__dtStartDate)

        self.__dtEndDate = QDateTimeEdit()
        self.__dtEndDate.setDisplayFormat("yyyy-MM-dd hh:mm")
        self.__dtEndDate.setCalendarPopup(True)
        self.__dtEndDate.setDateTime(QDateTime.currentDateTime())
        #self.__dtEndDate.dateTimeChanged.connect(self.onFilterChanged)
        dateLayout.addRow("End date: ", self.__dtEndDate)

        hDateLayout = QHBoxLayout(self.__gbDate)
        hDateLayout.setContentsMargins(5,5,5,5)
        hDateLayout.addLayout(dateLayout)
        hDateLayout.addStretch()

        hUpperLayout.addWidget(self.__gbDate)

        vFilterLayout.addLayout(hUpperLayout)

        hDateLine = QFrame(self)
        hDateLine.setFrameShape(QFrame.HLine)
        hDateLine.setFrameShadow(QFrame.Sunken)
        vFilterLayout.addWidget(hDateLine)

        # Filter buttons
        hFilterLayout = QHBoxLayout()
        hFilterLayout.setSpacing(25)
        self.__laFilter = QLabel("Filter message type: ")
        font = self.__laFilter.font()
        font.setBold(True)
        self.__laFilter.setFont(font)

        text = "Show debug messages"
        self.__pbFilterDebug = QToolButton()
        self.__pbFilterDebug.setIcon(QIcon(':log-debug'))
        self.__pbFilterDebug.setMinimumSize(32,32)
        self.__pbFilterDebug.setStatusTip(text)
        self.__pbFilterDebug.setToolTip(text)
        self.__pbFilterDebug.setCheckable(True)
        self.__pbFilterDebug.setChecked(True)
        #self.__pbFilterDebug.clicked.connect(self.onFilterChanged)

        text = "Show information messages"
        self.__pbFilterInfo = QToolButton()
        self.__pbFilterInfo.setIcon(QIcon(':log-info'))
        self.__pbFilterInfo.setMinimumSize(32,32)
        self.__pbFilterInfo.setStatusTip(text)
        self.__pbFilterInfo.setToolTip(text)
        self.__pbFilterInfo.setCheckable(True)
        self.__pbFilterInfo.setChecked(True)
        #self.__pbFilterInfo.clicked.connect(self.onFilterChanged)

        text = "Show warn messages"
        self.__pbFilterWarn = QToolButton()
        self.__pbFilterWarn.setIcon(QIcon(':log-warning'))
        self.__pbFilterWarn.setMinimumSize(32,32)
        self.__pbFilterWarn.setStatusTip(text)
        self.__pbFilterWarn.setToolTip(text)
        self.__pbFilterWarn.setCheckable(True)
        self.__pbFilterWarn.setChecked(True)
        #self.__pbFilterWarn.clicked.connect(self.onFilterChanged)

        text = "Show error messages"
        self.__pbFilterError = QToolButton()
        self.__pbFilterError.setIcon(QIcon(':log-error'))
        self.__pbFilterError.setMinimumSize(32,32)
        self.__pbFilterError.setStatusTip(text)
        self.__pbFilterError.setToolTip(text)
        self.__pbFilterError.setCheckable(True)
        self.__pbFilterError.setChecked(True)
        #self.__pbFilterError.clicked.connect(self.onFilterChanged)

        text = "Show alarm messages"
        self.__pbFilterAlarm = QToolButton()
        self.__pbFilterAlarm.setIcon(QIcon(':log-alarm'))
        self.__pbFilterAlarm.setMinimumSize(32,32)
        self.__pbFilterAlarm.setStatusTip(text)
        self.__pbFilterAlarm.setToolTip(text)
        self.__pbFilterAlarm.setCheckable(True)
        self.__pbFilterAlarm.setChecked(True)
        #self.__pbFilterAlarm.clicked.connect(self.onFilterChanged)

        text = "Show warning messages"
        self.__pbFilterWarning = QToolButton()
        self.__pbFilterWarning.setIcon(QIcon(':log-warning'))
        self.__pbFilterWarning.setMinimumSize(32,32)
        self.__pbFilterWarning.setStatusTip(text)
        self.__pbFilterWarning.setToolTip(text)
        self.__pbFilterWarning.setCheckable(True)
        self.__pbFilterWarning.setChecked(True)
        #self.__pbFilterWarning.clicked.connect(self.onFilterChanged)

        hFilterLayout.setContentsMargins(5,5,5,5)
        hFilterLayout.addWidget(self.__laFilter)
        hFilterLayout.addWidget(self.__pbFilterDebug)
        hFilterLayout.addWidget(self.__pbFilterInfo)
        hFilterLayout.addWidget(self.__pbFilterWarn)
        hFilterLayout.addWidget(self.__pbFilterError)
        hFilterLayout.addWidget(self.__pbFilterAlarm)
        hFilterLayout.addWidget(self.__pbFilterWarning)
        hFilterLayout.addStretch()

        if self.__isLogData:
            # Do not show these buttons
            self.__pbFilterDebug.setChecked(True)
            self.__pbFilterDebug.setVisible(True)
            self.__pbFilterInfo.setChecked(True)
            self.__pbFilterInfo.setVisible(True)
            self.__pbFilterWarn.setChecked(True)
            self.__pbFilterWarn.setVisible(True)
            self.__pbFilterError.setChecked(True)
            self.__pbFilterError.setVisible(True)
            # Show these buttons
            self.__pbFilterAlarm.setChecked(True)
            self.__pbFilterAlarm.setVisible(False)
            self.__pbFilterWarning.setChecked(True)
            self.__pbFilterWarning.setVisible(False)
        else:
            # Show these buttons
            self.__pbFilterDebug.setChecked(True)
            self.__pbFilterDebug.setVisible(False)
            self.__pbFilterInfo.setChecked(True)
            self.__pbFilterInfo.setVisible(False)
            self.__pbFilterWarn.setChecked(True)
            self.__pbFilterWarn.setVisible(False)
            self.__pbFilterError.setChecked(True)
            self.__pbFilterError.setVisible(False)
            # Do not show these buttons
            self.__pbFilterAlarm.setChecked(True)
            self.__pbFilterAlarm.setVisible(True)
            self.__pbFilterWarning.setChecked(True)
            self.__pbFilterWarning.setVisible(True)

        vFilterLayout.addLayout(hFilterLayout)

        return filterWidget


    def addLogMessage(self, logData):
        with QMutexLocker(self.__logDataMutex):
            self.__logDataQueue.put(logData)


### slots ###
    def onViewNeedsUpdate(self):
        print "onViewNeedsUpdate", self.__twLogTable.selectionModel
        #self.hViewState = self.__twLogTable.horizontalHeader().saveState()
        #self.vViewState = self.__twLogTable.verticalHeader().saveState()

        print "last selection", self.__twLogTable.lastItemSelection

        #self.onFilterChanged()
        queryText = "SELECT id, dateTime, messageType, instanceId, description, additionalDescription FROM tLog"
        with QMutexLocker(self.__modelMutex):
            self.sqlQueryModel.setLogQuery(queryText)

        if self.__twLogTable.lastItemSelection:
            #self.__twLogTable.selectionModel.select(self.__twLogTable.lastItemSelection,
            #                                        QItemSelectionModel.Select)
            self.__twLogTable.selectRow(self.__twLogTable.lastItemSelection.row())
            self.__twLogTable.scrollTo(self.__twLogTable.lastItemSelection,
                                       QAbstractItemView.PositionAtCenter)

        #self.__twLogTable.horizontalHeader().restoreState(self.vViewState)
        #self.__twLogTable.horizontalHeader().restoreState(self.hViewState)


    def onViewNeedsSortUpdate(self, queryText):
        #viewState = self.__twLogTable.horizontalHeader().saveState()
        with QMutexLocker(self.__modelMutex):
            self.sqlQueryModel.setLogQuery(queryText)
        #self.__twLogTable.horizontalHeader().restoreState(viewState)


    def onFilterChanged(self):
        filterQuery = ""

        msgTypeFilter = ""
        filterApplied = False
        if self.__isLogData:
            if self.__pbFilterDebug.isChecked():
                if filterApplied:
                    msgTypeFilter += " OR "
                msgTypeFilter += "messageType='DEBUG'"
                filterApplied = True
            if self.__pbFilterInfo.isChecked():
                if filterApplied:
                    msgTypeFilter += " OR "
                msgTypeFilter += "messageType='INFO'"
                filterApplied = True
            if self.__pbFilterWarn.isChecked():
                if filterApplied:
                    msgTypeFilter += " OR "
                msgTypeFilter += "messageType='WARN'"
                filterApplied = True
            if self.__pbFilterError.isChecked():
                if filterApplied:
                    msgTypeFilter += " OR "
                msgTypeFilter += "messageType='ERROR'"
                filterApplied = True
        else:
            if self.__pbFilterAlarm.isChecked():
                if filterApplied:
                    msgTypeFilter += " OR "
                msgTypeFilter += "messageType='ALARM_LOW' OR messageType='ALARM_HIGH'"
                filterApplied = True
            if self.__pbFilterWarning.isChecked():
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

        if self.__gbDate.isChecked():
            startDateTime = self.__dtStartDate.dateTime()
            startTime = startDateTime.time()
            startTime.setHMS(startTime.hour(), startTime.minute(), 0)
            startDateTime.setTime(startTime)
            endDateTime = self.__dtEndDate.dateTime()
            endTime = endDateTime.time()
            endTime.setHMS(endTime.hour(), endTime.minute(), 59)
            endDateTime.setTime(endTime)

            # Check start and end range
            if endDateTime < startDateTime:
                self.__dtStartDate.setDateTime(endDateTime)
                self.__dtEndDate.setDateTime(startDateTime)
                return

            if filterApplied:
                filterQuery += " AND "
            filterQuery += "(dateTime >= strftime('%Y-%m-%d %H:%M:%S', '" +startDateTime.toString(dateTimeFormat)+ "') AND " \
                            "dateTime <= strftime('%Y-%m-%d %H:%M:%S', '" +endDateTime.toString(dateTimeFormat)+ "')" \
                           ")"
            filterApplied = True

        # Text search options
        searchText = self.__leSearch.text()
        searchQuery = ""
        if len(searchText) > 0:
            if self.__pbSearchInsId.isChecked():
                if len(searchQuery) > 0:
                    searchQuery += " OR "
                searchQuery += "instanceId LIKE '%" +searchText+ "%'"
                filterApplied = True
            if self.__pbSearchDescr.isChecked():
                if len(searchQuery) > 0:
                    searchQuery += " OR "
                searchQuery += "description LIKE '%" +searchText+ "%'"
                filterApplied = True
            if self.__pbSearchAddDescr.isChecked():
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

        with QMutexLocker(self.__modelMutex):
            self.sqlQueryModel.setLogQuery(queryText)
        #self.__twLogTable.resizeColumnsToContents()
        #self.__twLogTable.resizeRowsToContents()


    def onFilterOptionVisible(self, checked):
        if checked:
            text = "Hide filter options"
            self.__pbFilterOptions.setText("- " + text)
        else:
            text = "Show filter options"
            self.__pbFilterOptions.setText("+ " + text)
        self.__pbFilterOptions.setStatusTip(text)
        self.__pbFilterOptions.setToolTip(text)

        self.__filterWidget.setVisible(checked)



class LogTableView(QTableView):


    def __init__(self, model, parent=None):
        super(LogTableView, self).__init__(parent)

        # Models
        self.setModel(model)

        # Selection
        self.selectionModel = self.selectionModel()
        self.selectionModel.selectionChanged.connect(self.onSelectionChanged)
        self.lastItemSelection = None

        self.setWordWrap(True)
        self.setAlternatingRowColors(True)
        self.horizontalHeader().setStretchLastSection(True)
        self.horizontalHeader().setResizeMode(QHeaderView.ResizeToContents)#Interactive)
        self.verticalHeader().setVisible(False)

        # Selection
        self.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.setSelectionMode(QAbstractItemView.SingleSelection)

        # Sorting
        self.setSortingEnabled(True)
        self.sortByColumn(0, Qt.DescendingOrder)


    # TODO: not working right now due to model-view in navigation
    def mouseDoubleClickEvent(self, event):
        index = self.model().index(self.currentIndex().row(), 3)
        value = index.data()
        # Emit signal with deviceId to select device instance
        Manager().signalSelectNewNavigationItem.emit("device." + value)
        QTableView.mouseDoubleClickEvent(self, event)


    def onSelectionChanged(self, selected, deselected):

        indexes = selected.indexes()
        nbIndexes = len(indexes)
        print "onSelectionChanged", nbIndexes, self.selectionModel
        if nbIndexes < 2:
            return

        #self.lastItemSelection = QItemSelection(indexes[0], indexes[1])
        self.lastItemSelection = indexes[1]
        print "self.lastItemSelection", self.lastItemSelection



class LogSqlQueryModel(QSqlQueryModel):
    # Define signals
    signalViewNeedsSortUpdate = pyqtSignal(str) # queryText

    def __init__(self, parent=None, preQueryText=str(), sortByColumn=0, sortOrder=Qt.DescendingOrder):
        super(LogSqlQueryModel, self).__init__(parent)

        self.preQueryText = preQueryText
        self.sortByColumn = sortByColumn
        self.sortOrder = sortOrder


    def setLogQuery(self, queryText=str()):
        self.setQuery(queryText)

        self.setHeaderData(0, Qt.Horizontal, "ID")
        self.setHeaderData(1, Qt.Horizontal, "Date and time")
        self.setHeaderData(2, Qt.Horizontal, "Message type")
        self.setHeaderData(3, Qt.Horizontal, "Instance ID")
        self.setHeaderData(4, Qt.Horizontal, "Description")
        self.setHeaderData(5, Qt.Horizontal, "Additional description")

        if self.lastError().isValid():
            print "Error", self.lastError()
        return

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

        print queryText
        print ""
        self.setQuery(queryText)

        self.setHeaderData(0, Qt.Horizontal, "ID")
        self.setHeaderData(1, Qt.Horizontal, "Date and time")
        self.setHeaderData(2, Qt.Horizontal, "Message type")
        self.setHeaderData(3, Qt.Horizontal, "Instance ID")
        self.setHeaderData(4, Qt.Horizontal, "Description")
        self.setHeaderData(5, Qt.Horizontal, "Additional description")


    def getIcon(self, value):
        if value == 'DEBUG':
            return QIcon(':log-debug')
        elif value == 'INFO':
            return QIcon(':log-info')
        elif (value == 'WARN') or (value == 'WARN_LOW') or (value == 'WARN_HIGH'):
            return QIcon(':log-warning')
        elif value == 'ERROR':
            return QIcon(':log-error')
        elif (value == 'ALARM_LOW') or (value == 'ALARM_HIGH'):
            return QIcon(':log-alarm')
        return None


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
            return self.getIcon(modelIndex.data(Qt.DisplayRole))
        elif role == Qt.TextColorRole and index.column() == 2:
            # Get text for comparison to get correct text color
            modelIndex = QSqlQueryModel.index(self, index.row(), index.column())
            return self.getTextColor(modelIndex.data(Qt.DisplayRole))
        elif role == Qt.DisplayRole:
            value = QSqlQueryModel.data(self, index, role)
            return value
        elif role == Qt.ToolTipRole:
            modelIndex = QSqlQueryModel.index(self, index.row(), index.column())
            return modelIndex.data(Qt.DisplayRole)
        return None


    #def sort(self, column, order):
    #    self.sortByColumn = column
    #    self.sortOrder = order
    #    self.setLogQuery(self.preQueryText)
        # Send signal to update view
    #    self.signalViewNeedsSortUpdate.emit(self.preQueryText)


class LogThread(QThread):
    # Define signals
    signalViewNeedsUpdate = pyqtSignal()

    def __init__(self, parent, model, logDataQueue, logDataMutex):
        super(LogThread, self).__init__(parent)

        self.sqlQueryModel = model

        self.modelMutex = QMutex()

        self.logDataQueue = logDataQueue
        self.logDataMutex = logDataMutex

        self.isFinished = False
        # Delete when no longer needed
        self.finished.connect(self.onDeleteLater)


    def getLogDataBlock(self):
        # Put all data from current queue into string
        logBlock = str()
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
                additionalDescription = str()
            self.insertInto(dateTime, logLevel, instanceId, description, additionalDescription)

        # Performance test
        #i = 50
        #while i > 0:
        #    self.insertInto(QDateTime.currentDateTime().toString(dateTimeFormat), "INFO", "pcx17673/DemoDevice/100", "This is short.", "LOW")
        #    i -= 1


    def insertInto(self, dateTime, msgType, instanceId, description, additionalDescription=str()):
        # Insert parameter into database
        queryText = "INSERT INTO tLog (dateTime, messageType, instanceId, description, additionalDescription) " \
                    "VALUES (strftime('%Y-%m-%d %H:%M:%S','" + dateTime + "'), '" +msgType+ "', '" +instanceId+ \
                    "', '" +description+ "', '" +additionalDescription+ "');"
        with QMutexLocker(self.modelMutex):
            self.sqlQueryModel.setQuery(queryText)


    def run(self):
        while not self.isFinished:
            if not self.logDataQueue.empty():
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

