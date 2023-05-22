#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on November 6, 2019
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
import weakref
from collections import namedtuple
from contextlib import contextmanager
from functools import partial

from qtpy.QtCore import (
    QAbstractTableModel, QModelIndex, QSortFilterProxyModel, Qt)
from qtpy.QtGui import QBrush, QColor
from qtpy.QtWidgets import (
    QHBoxLayout, QHeaderView, QLineEdit, QMessageBox, QPushButton, QTableView,
    QVBoxLayout, QWidget)
from traits.api import Instance, WeakRef

from karabo.common.scenemodel.api import DaemonManagerModel
from karabo.common.services import KARABO_DAEMON_MANAGER
from karabogui import messagebox
from karabogui.binding.api import VectorHashBinding, get_editor_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)
from karabogui.controllers.table.api import TableButtonDelegate
from karabogui.request import call_device_slot
from karabogui.singletons.api import get_topology
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


def request_daemon_action(serverId, hostId, action, parent):
    """Request an action for the daemon manager

    :param serverId: The targeted `serverId`
    :param hostId: The `hostId` of the server with `serverId`
    :param action: The action to be performed, e.g. `kill`, ...
    """
    device_id = KARABO_DAEMON_MANAGER
    device = get_topology().get_device(device_id)
    # XXX: Protect here if the device is offline. We share the same
    # logic as the device scene link!
    if device is not None and not device.online:
        parent = parent()
        messagebox.show_warning(f"Device <b>{device_id}</b> is not online!",
                                "Warning", parent=parent)
        return

    def handle_daemon_from_server(serverId, action, parent, success, reply):
        """Callback handler for a request the daemon manager"""
        parent = parent()
        if not success or not reply.get('payload.success', False):
            msg = 'The command "{}" for the server "{}" was not successful!'
            messagebox.show_warning(msg.format(action, serverId),
                                    title='Daemon Service Failed',
                                    parent=parent)
            return
        msg = 'The command "{}" for the server "{}" was successful!'
        messagebox.show_information(msg.format(action, serverId),
                                    title='Daemon Service Success!',
                                    parent=parent)

        return

    handler = partial(handle_daemon_from_server, serverId, action, parent)
    call_device_slot(handler, device_id, 'requestDaemonAction',
                     serverId=serverId, hostId=hostId, action=action)


def get_status_brush(service_status):
    status = service_status.split(',')[0]
    if status == "up":
        return QBrush(QColor(120, 255, 0))
    elif status == "down":
        return QBrush(QColor(255, 0, 0))


def get_duration_brush(service_duration):
    time = float(service_duration)
    return None if time > 1 else QBrush(QColor(255, 0, 0))


class DaemonServiceButton(TableButtonDelegate):

    def get_button_text(self, index):
        """Reimplemented function of `TableButtonDelegate` to display text"""
        return COLUMN_TEXT[index.column()]

    def click_action(self, index):
        """Reimplemented function of `TableButtonDelegate`"""
        if not index.isValid():
            return
        model = index.model()
        cmd = COMMANDS[index.column()]
        serviceId = model.index(index.row(), SERVER_COLUMN).data()
        hostId = model.index(index.row(), HOST_COLUMN).data()

        if cmd in ['down', 'kill']:
            text = (f'Are you sure you want to <b>{cmd}</b> the service '
                    f'<b>{serviceId}</b> on host <b>{hostId}</b>?')
            msg_box = QMessageBox(QMessageBox.Question, 'Daemon action',
                                  text, QMessageBox.Yes | QMessageBox.Cancel,
                                  parent=self.parent())
            msg_box.setDefaultButton(QMessageBox.Cancel)
            msg_box.setModal(False)
            move_to_cursor(msg_box)
            if msg_box.exec() != QMessageBox.Yes:
                return

        parent = weakref.ref(self.parent())
        request_daemon_action(serviceId, hostId, cmd, parent=parent)


class DaemonTableModel(QAbstractTableModel):
    """ A class which describes the relevant data (model) of a daemon manager
    device to show in a table view.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
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
                # XXX: The QAbstractItemModel does not update immediately when
                # the item delegate columns are included!
                row_begin = self.index(index, 0, QModelIndex())
                row_end = self.index(index, self.columnCount() - 3,
                                     QModelIndex())
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
    delegate = WeakRef(TableButtonDelegate)

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
        btn_delegate = DaemonServiceButton(parent=table_view)
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
        if self.delegate and self.delegate.isEnabled() != enable:
            with self.table_model.reset_context():
                self.delegate.setEnabled(enable)
