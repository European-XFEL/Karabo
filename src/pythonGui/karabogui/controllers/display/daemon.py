#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on November 6, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import namedtuple

from PyQt5.QtCore import (
    QAbstractTableModel, QModelIndex, QSortFilterProxyModel, Qt, pyqtSlot)
from PyQt5.QtGui import QBrush, QColor
from PyQt5.QtWidgets import (
    QHBoxLayout, QHeaderView, QLineEdit, QTableView, QStyledItemDelegate,
    QPushButton, QStyle, QVBoxLayout, QWidget)

from traits.api import Instance

from karabo.common.scenemodel.api import DaemonManagerModel
from karabogui.binding.api import VectorHashBinding, get_editor_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)
from karabogui.util import request_daemon_action

SERVER_COLUMN = 0
STATUS_COLUMN = 1
HOST_COLUMN = 4
START_COLUMN = 5
STOP_COLUMN = 6
KILL_COLUMN = 7

COLUMN_TEXT = {
    0: "Name",
    1: "Status",
    2: "Since",
    3: "Duration",
    4: "Host",
    5: "Start",
    6: "Stop",
    7: "Kill"
}

COMMANDS = {
    5: "up",
    6: "down",
    7: "kill"
}

HEADER_LABELS = [text for text in COLUMN_TEXT.values()]
ENTRY_LABELS = [text.lower() for column, text
                in COLUMN_TEXT.items() if column < 5]

serviceEntry = namedtuple('serviceEntry', ENTRY_LABELS)


def get_brush(service_status):
    status = service_status.split(',')[0]
    if status == "up":
        return QBrush(QColor(120, 255, 0))
    elif status == "down":
        return QBrush(QColor(255, 0, 0))


class DaemonTableModel(QAbstractTableModel):
    """ A class which describes the relevant data (model) of a daemon manager
    device to show in a table view.
    """

    def __init__(self, parent=None):
        super(DaemonTableModel, self).__init__(parent)
        self._table_data = []

    def initialize(self, value):
        self.beginResetModel()
        for index, row_data in enumerate(value):
            self._table_data[index] = serviceEntry(**row_data)
        self.endResetModel()

    def update_model(self, value):
        num_rows = self.rowCount()
        new_rows = len(value)
        difference = new_rows - num_rows

        # Update our book keeping Hash first before proceeding!
        for index, row_data in enumerate(value):
            if index < num_rows:
                self._table_data[index] = serviceEntry(**row_data)
                row_begin = self.index(index, self.columnCount(),
                                       QModelIndex())
                row_end = self.index(index, self.columnCount(), QModelIndex())
                self.dataChanged.emit(row_begin, row_end)
            else:
                row = self.rowCount()
                self.beginInsertRows(QModelIndex(), row, row)
                self._table_data.append(serviceEntry(**row_data))
                self.endInsertRows()

        if difference < 0:
            for _ in range(abs(difference)):
                # NOTE: We can safely pop the data, since the update
                # overwrites! The rows start at 0!
                row = self.rowCount()
                self.beginRemoveRows(QModelIndex(), row - 1, row)
                self._table_data.pop()
                self.endRemoveRows()

    def headerData(self, section, orientation, role):
        if role == Qt.DisplayRole and orientation == Qt.Horizontal:
            return HEADER_LABELS[section]

    def rowCount(self, parent=QModelIndex()):
        return len(self._table_data)

    def columnCount(self, parent=QModelIndex()):
        return len(HEADER_LABELS)

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None
        entry = self._table_data[index.row()]
        if role in (Qt.DisplayRole, Qt.ToolTipRole):
            if index.column() < len(ENTRY_LABELS):
                return str(entry[index.column()])
        elif role == Qt.BackgroundRole and index.column() == STATUS_COLUMN:
            return get_brush(entry.status)

        return None


_is_compatible = with_display_type('DaemonManager')


@register_binding_controller(ui_name='Daemon Manager', can_edit=False,
                             klassname='DaemonManager',
                             binding_type=VectorHashBinding,
                             is_compatible=_is_compatible, priority=0)
class DisplayDaemonService(BaseBindingController):
    model = Instance(DaemonManagerModel, args=())
    table_model = Instance(QAbstractTableModel)

    def create_widget(self, parent):
        widget = QWidget(parent)
        layout = QVBoxLayout(widget)
        widget.setLayout(layout)

        # The main table view!
        table_view = QTableView(widget)
        self.table_model = DaemonTableModel(parent=table_view)

        # Set up the filter model!
        filter_model = QSortFilterProxyModel(parent=table_view)
        filter_model.setSourceModel(self.table_model)
        filter_model.setFilterRole(Qt.DisplayRole)
        filter_model.setFilterCaseSensitivity(False)
        filter_model.setFilterFixedString("")
        filter_model.setFilterKeyColumn(0)

        table_view.setModel(filter_model)
        btn_delegate = ButtonDelegate(parent=table_view)
        table_view.setItemDelegateForColumn(START_COLUMN, btn_delegate)
        table_view.setItemDelegateForColumn(STOP_COLUMN, btn_delegate)
        table_view.setItemDelegateForColumn(KILL_COLUMN, btn_delegate)

        header = table_view.horizontalHeader()
        header.setDefaultSectionSize(50)
        header.setResizeMode(QHeaderView.ResizeToContents)
        header.setSectionResizeMode(0, QHeaderView.Stretch)
        header.setSectionResizeMode(5, QHeaderView.Fixed)
        header.setSectionResizeMode(6, QHeaderView.Fixed)
        header.setSectionResizeMode(7, QHeaderView.Fixed)

        search_layout = QHBoxLayout()
        search_line = QLineEdit(parent=widget)
        clear_button = QPushButton("Clear", parent=widget)
        clear_button.clicked.connect(search_line.clear)
        search_line.textChanged.connect(filter_model.setFilterFixedString)

        search_layout.addWidget(search_line)
        search_layout.addWidget(clear_button)

        layout.addLayout(search_layout)
        layout.addWidget(table_view)

        return widget

    def value_update(self, proxy):
        value = get_editor_value(proxy, [])
        self.table_model.update_model(value)


class ButtonDelegate(QStyledItemDelegate):
    def __init__(self, parent=None):
        super(ButtonDelegate, self).__init__(parent)
        self._button = QPushButton("")
        self._button.hide()
        parent.clicked.connect(self.cellClicked)

    def _is_relevant_column(self, index):
        column = index.column()
        if column in [START_COLUMN, STOP_COLUMN, KILL_COLUMN]:
            return True, COLUMN_TEXT[column]

        return False, ""

    def createEditor(self, parent, option, index):
        """This method is called whenever the delegate is in edit mode."""
        relevant, text = self._is_relevant_column(index)
        if relevant:
            # This button is for the highlighting effect when clicking/editing
            button = QPushButton(parent)
            button.setText(text)
            return button
        else:
            return super(ButtonDelegate, self).createEditor(parent, option,
                                                            index)

    def updateEditorGeometry(self, button, option, index):
        relevant, text = self._is_relevant_column(index)
        if relevant:
            button.setGeometry(option.rect)
            button.setText(text)

    def setEditorData(self, button, index):
        relevant, text = self._is_relevant_column(index)
        if relevant:
            button.setText(text)
        else:
            super(ButtonDelegate, self).setEditorData(button, index)

    def paint(self, painter, option, index):
        relevant, text = self._is_relevant_column(index)
        if relevant:
            self._button.setGeometry(option.rect)
            self._button.setText(text)
            if option.state == QStyle.State_Selected:
                painter.fillRect(option.rect, option.palette.highlight())
            pixmap = self._button.grab()
            painter.drawPixmap(option.rect.x(), option.rect.y(), pixmap)
        else:
            super(ButtonDelegate, self).paint(painter, option, index)

    @pyqtSlot(QModelIndex)
    def cellClicked(self, index):
        if not index.isValid():
            return
        relevant, _ = self._is_relevant_column(index)
        if relevant:
            model = index.model()
            serviceId = model.index(index.row(), SERVER_COLUMN).data()
            hostId = model.index(index.row(), HOST_COLUMN).data()
            cmd = COMMANDS[index.column()]
            request_daemon_action(serviceId, hostId, cmd)
