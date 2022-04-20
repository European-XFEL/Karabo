#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 4, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import namedtuple

from qtpy.QtCore import (
    QAbstractTableModel, QDateTime, QModelIndex, QPoint, QSortFilterProxyModel,
    Qt, Slot)
from qtpy.QtGui import QClipboard, QColor
from qtpy.QtWidgets import (
    QAbstractItemView, QApplication, QHBoxLayout, QLineEdit, QMenu,
    QPushButton, QTableView, QVBoxLayout, QWidget)

from karabogui import icons
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.singletons.api import get_config
from karabogui.util import getSaveFileName

TYPE_COLUMN = 1
INSTANCE_COLUMN = 2
MAX_LOG_ENTRIES = 300


class LogWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        # Main layout
        vertical_layout = QVBoxLayout(self)
        vertical_layout.setContentsMargins(0, 0, 0, 0)
        vertical_layout.setSpacing(2)

        filter_layout = QHBoxLayout()
        filter_layout.setContentsMargins(2, 2, 2, 2)
        vertical_layout.addLayout(filter_layout)

        text = "Search"
        filter_text = QLineEdit()
        filter_text.setStatusTip(text)
        filter_text.setToolTip(text)
        filter_layout.addWidget(filter_text)

        clear_button = QPushButton("Clear Filter")
        filter_layout.addWidget(clear_button)

        self.table_model = TableLogModel()
        self.filter_model = QSortFilterProxyModel()
        self.filter_model.setFilterCaseSensitivity(False)
        self.filter_model.setFilterRole(Qt.DisplayRole)
        self.filter_model.setFilterKeyColumn(INSTANCE_COLUMN)

        self.filter_model.setSourceModel(self.table_model)

        clear_button.clicked.connect(filter_text.clear)
        filter_text.textChanged.connect(self.filter_model.setFilterFixedString)

        table_view = QTableView(parent=self)
        vertical_layout.addWidget(table_view)
        table_view.setModel(self.filter_model)
        table_view.setWordWrap(True)
        table_view.setAlternatingRowColors(True)
        table_view.horizontalHeader().setStretchLastSection(True)
        table_view.verticalHeader().setVisible(False)
        table_view.setSelectionBehavior(QAbstractItemView.SelectRows)
        table_view.setSelectionMode(QAbstractItemView.SingleSelection)
        table_view.setSortingEnabled(True)
        table_view.sortByColumn(0, Qt.DescendingOrder)
        table_view.doubleClicked.connect(self.onItemDoubleClicked)
        table_view.setContextMenuPolicy(Qt.CustomContextMenu)
        table_view.customContextMenuRequested.connect(self._context_menu)
        self.table = table_view
        self.setLayout(vertical_layout)

    @Slot(QPoint)
    def _context_menu(self, pos):
        """The custom context menu of a reconfigurable table element"""
        index = self.table.selectionModel().currentIndex()
        menu = QMenu()
        if index.isValid():
            copy_action = menu.addAction("Copy to clipboard")
            copy_action.triggered.connect(self._copy_clipboard)
            menu.exec(self.table.viewport().mapToGlobal(pos))

    @Slot()
    def _copy_clipboard(self):
        index = self.table.selectionModel().currentIndex()
        if index.isValid():
            index = self.filter_model.mapToSource(index)
            model = self.table_model
            time = model.data(index.sibling(index.row(), 1),
                              Qt.DisplayRole)
            logtype = model.data(index.sibling(index.row(), 2),
                                 Qt.DisplayRole)
            instance_id = model.data(index.sibling(index.row(), 3),
                                     Qt.DisplayRole)
            exception = model.data(index.sibling(index.row(), 4),
                                   Qt.DisplayRole)
            description = model.data(index.sibling(index.row(), 5),
                                     Qt.DisplayRole)
            log = (f"- {time} --- {logtype} --- {instance_id} -\n\n-----\n"
                   f"{exception}\n{description}")
            clipboard = QApplication.clipboard()
            clipboard.clear(mode=QClipboard.Clipboard)
            clipboard.setText(log, mode=QClipboard.Clipboard)

    def initialize(self, logData):
        new = [Log(messageType=log["type"], instanceId=log["category"],
                   description=log["message"],
                   traceback=log.get("traceback", ""),
                   dateTime=QDateTime.fromString(
                       log["timestamp"], Qt.ISODate).toString(Qt.ISODate))
               for log in logData]
        self.table_model.initialize(new)

    def onLogDataAvailable(self, logData):
        new = [Log(messageType=log["type"], instanceId=log["category"],
                   description=log["message"],
                   traceback=log.get("traceback", ""),
                   dateTime=QDateTime.fromString(
                       log["timestamp"], Qt.ISODate).toString(Qt.ISODate))
               for log in logData]

        model = self.table_model
        model.add(new)
        # If we are full we will remove data
        difference = model.rowCount() - MAX_LOG_ENTRIES
        if difference > 0:
            self.table_model.prune(difference)

    @Slot(QModelIndex)
    def onItemDoubleClicked(self, index):
        value = index.data()
        if value is None:
            return
        source_index = self.filter_model.mapToSource(index)
        index = self.table_model.index(source_index.row(), INSTANCE_COLUMN)
        deviceId = self.table_model.data(index, Qt.DisplayRole)
        broadcast_event(KaraboEvent.ShowDevice, {"deviceId": deviceId})

    @Slot()
    def onClearLog(self):
        self.table_model.setList([])

    @Slot()
    def onSaveToFile(self):
        """Write current database content to a file """
        filename = getSaveFileName(
            caption="Save file as",
            filter="Log files (*.log)",
            suffix="log",
            directory=get_config()["data_dir"],
            parent=self)
        if not filename:
            return

        with open(filename, "w") as out:
            for log in self.table_model.getData():
                out.write("{0} | {1} | {2} | {3} | {4} #\n".format(*log))


Log = namedtuple("Log", ["dateTime", "messageType", "instanceId",
                         "description", "traceback"])


class TableLogModel(QAbstractTableModel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self._data = []

    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return ("Timestamp", "Type", "Instance ID",
                    "Description", "Traceback")[section]

    def setList(self, data):
        self.beginResetModel()
        self._data = data
        self.endResetModel()

    def getData(self):
        return self._data

    def rowCount(self, parent=QModelIndex()):
        return len(self._data)

    def columnCount(self, parent=QModelIndex()):
        return 5

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

        log = self._data[index.row()]
        if role == Qt.DecorationRole and index.column() == TYPE_COLUMN:
            return self.icons.get(log.messageType)
        elif role == Qt.TextColorRole and index.column() == TYPE_COLUMN:
            return self.textColor.get(log.messageType)
        elif role in (Qt.DisplayRole, Qt.ToolTipRole):
            return log[index.column()]
        return None

    def initialize(self, data):
        self.beginResetModel()
        try:
            self._data = data
        finally:
            self.endResetModel()

    def add(self, data):
        self.beginInsertRows(QModelIndex(), 0, len(data) - 1)
        try:
            for log in data:
                self._data.insert(0, log)
        finally:
            self.endInsertRows()

    def prune(self, index):
        self.beginResetModel()
        # Keep the latest data until index
        self._data = self._data[:index]
        self.endResetModel()
