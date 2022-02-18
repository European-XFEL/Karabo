from collections import namedtuple

from qtpy import uic
from qtpy.QtCore import (
    QAbstractTableModel, QDateTime, QModelIndex, QPoint, QSize, Qt, Slot)
from qtpy.QtGui import QClipboard, QColor
from qtpy.QtWidgets import (
    QAbstractItemView, QApplication, QDialog, QHeaderView, QMenu)

from karabogui import icons, messagebox
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.request import call_device_slot
from karabogui.util import WeakMethodRef, get_spin_widget

from .utils import get_dialog_ui

LogData = namedtuple("LogData", "timestamp priority instanceId "
                                "description traceback")

INSTANCE_ID_COLUMN = 2
MAX_COLUMN_SIZE = 500

ICONS = {
    "DEBUG": icons.logDebug,
    "INFO": icons.logInfo,
    "WARN": icons.logWarning,
    "ERROR": icons.logError}

TEXT_COLOR = {
    "DEBUG": QColor(0, 128, 0),
    "INFO": QColor(0, 0, 128),
    "WARN": QColor(255, 102, 0),
    "ERROR": QColor(128, 0, 0)}


class LogDialog(QDialog):
    def __init__(self, server_id, parent=None):
        super().__init__(parent)
        uic.loadUi(get_dialog_ui("log_dialog.ui"), self)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        self.setSizeGripEnabled(True)
        self.setWindowTitle(f"Server log for serverId: {server_id}")

        self.server_id = server_id

        flags = Qt.WindowCloseButtonHint | Qt.WindowTitleHint
        self.setWindowFlags(self.windowFlags() | flags)

        self.spin_widget = get_spin_widget(icon="wait-black",
                                           scaled_size=QSize(16, 16))
        self.spin_widget.setVisible(False)
        self.bottom_layout.insertWidget(0, self.spin_widget)

        self.model = LogDataModel()

        self.ui_server_id.setText(server_id)
        self.ui_table_view.setModel(self.model)
        self.ui_table_view.setWordWrap(True)
        self.ui_table_view.setAlternatingRowColors(True)
        self.ui_table_view.horizontalHeader().setStretchLastSection(True)
        self.ui_table_view.verticalHeader().setVisible(False)

        self.ui_table_view.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.ui_table_view.setSelectionMode(QAbstractItemView.SingleSelection)
        self.ui_table_view.sortByColumn(0, Qt.DescendingOrder)

        self.ui_table_view.doubleClicked.connect(self.onItemDoubleClicked)

        self.ui_table_view.setContextMenuPolicy(Qt.CustomContextMenu)
        self.ui_table_view.customContextMenuRequested.connect(
            self._context_menu)
        self.table = self.ui_table_view

        self.ui_fetching_data.setVisible(False)
        header = self.ui_table_view.horizontalHeader()
        header.sectionDoubleClicked.connect(self.resize_contents)
        self.request_logger_data()

    def request_handler(self, success, reply):
        self.spin_widget.setVisible(False)
        self.ui_fetching_data.setVisible(False)
        if not success:
            messagebox.show_error(
                f"Could not fetch the logs of server <b>{self.server_id}</b>.",
                parent=self)
            return

        data = reply["content"]
        new = [LogData(priority=log["type"],
                       instanceId=log["category"],
                       description=log["message"],
                       traceback=log.get("traceback", ""),
                       timestamp=QDateTime.fromString(
                           log["timestamp"], Qt.ISODate).toString(Qt.ISODate))
               for log in data]

        self.model.initialize(new)
        self.resize_contents()

    def keyPressEvent(self, event):
        if (event.key() in (Qt.Key_Enter, Qt.Key_Return) and
                self.focusWidget() == self.ui_number_logs):
            self.request_logger_data()
            event.accept()
            return
        return super().keyPressEvent(event)

    @Slot()
    def resize_contents(self):
        last_column = self.model.columnCount() - 1
        hor_header = self.ui_table_view.horizontalHeader()
        width = hor_header.defaultSectionSize()
        hor_header.setMinimumSectionSize(width)
        hor_header.setMaximumSectionSize(MAX_COLUMN_SIZE)
        hor_header.resizeSections(QHeaderView.ResizeToContents)
        hor_header.resizeSection(last_column, QHeaderView.Stretch)

        ver_header = self.ui_table_view.verticalHeader()
        height = ver_header.defaultSectionSize()
        ver_header.setMinimumSectionSize(height)
        ver_header.resizeSections(QHeaderView.ResizeToContents)

    @Slot(QModelIndex)
    def onItemDoubleClicked(self, index):
        value = index.data()
        if value is None:
            return
        index = self.model.index(index.row(), INSTANCE_ID_COLUMN)
        deviceId = self.model.data(index, Qt.DisplayRole)
        broadcast_event(KaraboEvent.ShowDevice, {"deviceId": deviceId})

    @Slot()
    def request_logger_data(self):
        self.spin_widget.setVisible(True)
        self.ui_fetching_data.setVisible(True)
        call_device_slot(WeakMethodRef(self.request_handler), self.server_id,
                         "slotLoggerContent",
                         logs=self.ui_number_logs.value())

    @Slot(QPoint)
    def _context_menu(self, pos):
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
            model = index.model()
            time = model.data(index.sibling(index.row(), 0),
                              Qt.DisplayRole)
            logtype = model.data(index.sibling(index.row(), 1),
                                 Qt.DisplayRole)
            instance_id = model.data(index.sibling(index.row(), 2),
                                     Qt.DisplayRole)
            exception = model.data(index.sibling(index.row(), 3),
                                   Qt.DisplayRole)
            description = model.data(index.sibling(index.row(), 4),
                                     Qt.DisplayRole)
            log = (f"- {time} --- {logtype} --- {instance_id} -\n\n-----\n"
                   f"{exception}\n{description}")
            clipboard = QApplication.clipboard()
            clipboard.clear(mode=QClipboard.Clipboard)
            clipboard.setText(log, mode=QClipboard.Clipboard)


class LogDataModel(QAbstractTableModel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self._data = []

    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return ("Timestamp", "Priority", "Instance Id",
                    "Description", "Additional description")[section]

    def rowCount(self, index=QModelIndex()):
        return len(self._data)

    def columnCount(self, index=QModelIndex()):
        return 5

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None
        log = self._data[index.row()]
        if role == Qt.DecorationRole and index.column() == 1:
            return ICONS.get(log.priority)
        elif role == Qt.TextColorRole and index.column() == 1:
            return TEXT_COLOR.get(log.priority)
        elif role in (Qt.DisplayRole, Qt.ToolTipRole):
            return log[index.column()]
        elif (role == Qt.TextAlignmentRole
                and index.column() <= INSTANCE_ID_COLUMN):
            return Qt.AlignCenter

        return None

    def initialize(self, data):
        self.beginResetModel()
        self._data = data
        self.endResetModel()
