#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 4, 2012
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from collections import namedtuple

from qtpy.QtCore import (
    QAbstractTableModel, QDateTime, QModelIndex, QPoint, QSortFilterProxyModel,
    Qt, Slot)
from qtpy.QtGui import QClipboard, QColor
from qtpy.QtWidgets import (
    QAbstractItemView, QApplication, QFrame, QHBoxLayout, QHeaderView, QLabel,
    QLayout, QLineEdit, QMenu, QPushButton, QTableView, QToolButton,
    QVBoxLayout, QWidget)

from karabogui import icons
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.singletons.api import get_config
from karabogui.util import getSaveFileName

TYPE_COLUMN = 1
INSTANCE_COLUMN = 2
MAX_LOG_ENTRIES = 300
MIN_ROW_SIZE = 30
MAX_ROW_SIZE = 200
MAX_COLUMN_SIZE = 500

Log = namedtuple("Log", ["dateTime", "messageType", "instanceId",
                         "description", "traceback"])


class LogWidget(QWidget):
    def __init__(self, resize_contents=False, parent=None):
        super().__init__(parent)
        self.resize_contents = resize_contents

        # Main layout
        vertical_layout = QVBoxLayout(self)
        vertical_layout.setSizeConstraint(QLayout.SetNoConstraint)
        vertical_layout.setContentsMargins(0, 0, 0, 0)
        vertical_layout.setSpacing(4)

        filter_layout = QHBoxLayout()
        filter_layout.setContentsMargins(2, 2, 2, 2)
        vertical_layout.addLayout(filter_layout)

        line = QFrame()
        line.setFrameShape(QFrame.HLine)
        line.setFrameShadow(QFrame.Sunken)
        line.setLineWidth(2)
        vertical_layout.addWidget(line)

        text = "Search"
        filter_text = QLineEdit()
        filter_text.setStatusTip(text)
        filter_text.setToolTip(text)
        filter_layout.addWidget(filter_text)
        self.filter_text = filter_text

        clear_button = QPushButton("Clear Filter")
        filter_layout.addWidget(clear_button)

        # Filter buttons
        button_layout = QHBoxLayout()
        button_layout.setSpacing(25)
        vertical_layout.addLayout(button_layout)

        text = "Category Filter"
        label = QLabel()
        font = label.font()
        font.setBold(True)
        label.setFont(font)
        label.setText(text)
        label.setMinimumSize(32, 32)
        label.setStatusTip(text)
        label.setToolTip(text)

        text = "Show debug messages"
        self.button_debug = QToolButton()
        self.button_debug.setIcon(icons.logDebug)
        self.button_debug.setMinimumSize(32, 32)
        self.button_debug.setStatusTip(text)
        self.button_debug.setToolTip(text)
        self.button_debug.setCheckable(True)
        self.button_debug.setChecked(True)
        self.button_debug.clicked.connect(self.onFilterChanged)

        text = "Show information messages"
        self.button_info = QToolButton()
        self.button_info.setIcon(icons.logInfo)
        self.button_info.setMinimumSize(32, 32)
        self.button_info.setStatusTip(text)
        self.button_info.setToolTip(text)
        self.button_info.setCheckable(True)
        self.button_info.setChecked(True)
        self.button_info.clicked.connect(self.onFilterChanged)

        text = "Show warn messages"
        self.button_warn = QToolButton()
        self.button_warn.setIcon(icons.logWarning)
        self.button_warn.setMinimumSize(32, 32)
        self.button_warn.setStatusTip(text)
        self.button_warn.setToolTip(text)
        self.button_warn.setCheckable(True)
        self.button_warn.setChecked(True)
        self.button_warn.clicked.connect(self.onFilterChanged)

        text = "Show error messages"
        self.button_error = QToolButton()
        self.button_error.setIcon(icons.logError)
        self.button_error.setMinimumSize(32, 32)
        self.button_error.setStatusTip(text)
        self.button_error.setToolTip(text)
        self.button_error.setCheckable(True)
        self.button_error.setChecked(True)
        self.button_error.clicked.connect(self.onFilterChanged)

        text = "Resize to Contents"
        resize_button = QPushButton()
        resize_button.setStatusTip(text)
        resize_button.setToolTip(text)
        resize_button.setIcon(icons.resize)
        resize_button.clicked.connect(self._resize_contents)
        resize_button.setMinimumSize(32, 32)

        button_layout.setContentsMargins(0, 0, 0, 0)
        button_layout.addWidget(label)
        button_layout.addWidget(self.button_debug)
        button_layout.addWidget(self.button_info)
        button_layout.addWidget(self.button_warn)
        button_layout.addWidget(self.button_error)
        button_layout.addStretch()
        button_layout.addWidget(resize_button)

        self.table_model = TableLogModel()
        self.filter_model = LogFilterModel()
        self.filter_model.setFilterCaseSensitivity(False)
        self.filter_model.setFilterRole(Qt.DisplayRole)
        self.filter_model.setFilterKeyColumn(INSTANCE_COLUMN)
        self.filter_model.setSourceModel(self.table_model)

        clear_button.clicked.connect(filter_text.clear)
        filter_text.textChanged.connect(self.onFilterChanged)

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

    def _convert_log_data(self, data):
        """Convert log data coming from the network"""
        log_data = [Log(messageType=log["type"],
                        instanceId=log["category"],
                        description=log["message"],
                        traceback=log.get("traceback", ""),
                        dateTime=QDateTime.fromString(
                            log["timestamp"], Qt.ISODate).toString(Qt.ISODate))
                    for log in data]
        return log_data

    def initialize(self, data):
        new = self._convert_log_data(data)
        self.table_model.initialize(new)
        if self.resize_contents:
            self._resize_contents()

    def onLogDataAvailable(self, data):
        new = self._convert_log_data(data)
        model = self.table_model
        model.add(new)
        # If we are full we will remove data
        difference = model.rowCount() - MAX_LOG_ENTRIES
        if difference > 0:
            model.prune(difference)

    # ---------------------------------------------------------------------
    # Slots

    @Slot()
    def _resize_contents(self):
        """Resize columns to contents"""
        columns = self.table_model.columnCount() - 1
        hor_header = self.table.horizontalHeader()
        hor_header.setMaximumSectionSize(MAX_COLUMN_SIZE)
        hor_header.resizeSections(QHeaderView.ResizeToContents)
        hor_header.resizeSection(columns, QHeaderView.Stretch)

        ver_header = self.table.verticalHeader()
        ver_header.setMinimumSectionSize(MIN_ROW_SIZE)
        ver_header.setMaximumSectionSize(MAX_ROW_SIZE)
        ver_header.resizeSections(QHeaderView.ResizeToContents)

    @Slot()
    def onFilterChanged(self):
        filter_types = []
        if self.button_error.isChecked():
            filter_types.append("ERROR")
        if self.button_warn.isChecked():
            filter_types.append("WARN")
        if self.button_info.isChecked():
            filter_types.append("INFO")
        if self.button_debug.isChecked():
            filter_types.append("DEBUG")
        self.filter_model.filter_type = filter_types
        self.filter_model.setFilterFixedString(self.filter_text.text())
        if self.resize_contents:
            self._resize_contents()

    @Slot(QPoint)
    def _context_menu(self, pos):
        """The custom context menu of the log table"""
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
                out.write("{} | {} | {} | {} | {} #\n".format(*log))


class LogFilterModel(QSortFilterProxyModel):
    def __init__(self, source_model=None, parent=None):
        super().__init__(parent)
        self.setSourceModel(source_model)
        self.filter_type = ["INFO", "WARN", "ERROR", "DEBUG"]

    def filterAcceptsRow(self, row, parent=QModelIndex):
        model = self.sourceModel()
        category = model.data(model.index(row, TYPE_COLUMN))
        if category not in self.filter_type:
            return False
        return super().filterAcceptsRow(row, parent)


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
                 WARN=icons.logWarning, FATAL=icons.logError,
                 ERROR=icons.logError)

    textColor = dict(DEBUG=QColor(0, 128, 0), INFO=QColor(0, 0, 128),
                     WARN=QColor(255, 102, 0), FATAL=QColor(128, 0, 0),
                     ERROR=QColor(128, 0, 0))

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
