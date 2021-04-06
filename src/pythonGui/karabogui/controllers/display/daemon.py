#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on November 6, 2019
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import namedtuple
from contextlib import contextmanager
import weakref

from qtpy.QtCore import (
    QAbstractTableModel, QModelIndex, QSortFilterProxyModel, Qt, Slot)
from qtpy.QtGui import QBrush, QColor
from qtpy.QtWidgets import (
    QHBoxLayout, QHeaderView, QLineEdit, QMessageBox, QPushButton, QStyle,
    QStyledItemDelegate, QTableView, QVBoxLayout, QWidget)

from traits.api import Instance, WeakRef

from karabo.common.scenemodel.api import DaemonManagerModel
from karabogui.binding.api import VectorHashBinding, get_editor_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)
from karabogui.request import request_daemon_action
from karabogui.util import move_to_cursor

SERVER_COLUMN = 0
STATUS_COLUMN = 1
DURATION_COLUMN = 3
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


def get_status_brush(service_status):
    status = service_status.split(',')[0]
    if status == "up":
        return QBrush(QColor(120, 255, 0))
    elif status == "down":
        return QBrush(QColor(255, 0, 0))


def get_duration_brush(service_duration):
    time = int(service_duration)
    return None if time > 1 else QBrush(QColor(255, 0, 0))


class ButtonDelegate(QStyledItemDelegate):
    def __init__(self, parent=None):
        super(ButtonDelegate, self).__init__(parent)
        self.clickable = True

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
            self._button.setEnabled(self.clickable)
            painter.drawPixmap(option.rect.x(), option.rect.y(), pixmap)
        else:
            super(ButtonDelegate, self).paint(painter, option, index)

    @Slot(QModelIndex)
    def cellClicked(self, index):
        if not index.isValid() or not self.clickable:
            return
        relevant, _ = self._is_relevant_column(index)
        if relevant:
            model = index.model()
            cmd = COMMANDS[index.column()]
            serviceId = model.index(index.row(), SERVER_COLUMN).data()
            hostId = model.index(index.row(), HOST_COLUMN).data()

            if cmd in ['down', 'kill']:
                text = ('Are you sure you want to <b>{}</b> the service '
                        '<b>{}</b> on host <b>{}</b>?'.format(cmd,
                                                              serviceId,
                                                              hostId))
                msg_box = QMessageBox(QMessageBox.Question, 'Daemon action',
                                      text,
                                      QMessageBox.Yes | QMessageBox.Cancel,
                                      parent=self.parent())
                msg_box.setDefaultButton(QMessageBox.Cancel)
                msg_box.setModal(False)
                move_to_cursor(msg_box)
                if msg_box.exec_() != QMessageBox.Yes:
                    return

            parent = weakref.ref(self.parent())
            request_daemon_action(serviceId, hostId, cmd, parent)


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

        # XXX: We are nifty here and simply announce a complete layoutChange
        # This turns out to be several times faster than doing a dataChange
        # for every item. Avoid races by doing this close together...
        self.layoutAboutToBeChanged.emit()
        self.layoutChanged.emit()

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
        elif role == Qt.BackgroundRole:
            column = index.column()
            if column == STATUS_COLUMN:
                return get_status_brush(entry.status)
            elif column == DURATION_COLUMN:
                return get_duration_brush(entry.duration)

        return None

    @contextmanager
    def reset_context(self):
        try:
            self.beginResetModel()
            yield
        finally:
            self.endResetModel()


_is_compatible = with_display_type('DaemonManager')


@register_binding_controller(ui_name='Daemon Manager', can_edit=False,
                             klassname='DaemonManager',
                             binding_type=VectorHashBinding,
                             is_compatible=_is_compatible, priority=0)
class DisplayDaemonService(BaseBindingController):
    model = Instance(DaemonManagerModel, args=())
    table_model = WeakRef(QAbstractTableModel)
    delegate = WeakRef(ButtonDelegate)

    def create_widget(self, parent):
        widget = QWidget(parent)
        layout = QVBoxLayout(widget)
        widget.setLayout(layout)

        # The main table view!
        table_view = QTableView(widget)
        table_view.setSortingEnabled(True)
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
        self.delegate = btn_delegate

        header = table_view.horizontalHeader()
        header.setDefaultSectionSize(50)
        header.setSectionResizeMode(QHeaderView.ResizeToContents)
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

    def setEnabled(self, enable):
        """Reimplemented function of the base binding controller

        We enable and disable the action button on access level change!
        """
        if self.delegate and self.delegate.clickable != enable:
            with self.table_model.reset_context():
                self.delegate.clickable = enable
