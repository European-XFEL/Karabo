#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on June 16, 2025
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
from typing import NamedTuple

import natsort
from qtpy import uic
from qtpy.QtCore import (
    QAbstractTableModel, QEvent, QModelIndex, QSortFilterProxyModel, Qt, Slot)
from qtpy.QtWidgets import (
    QAbstractItemView, QApplication, QDialog, QHeaderView, QStyle,
    QStyledItemDelegate, QStyleOptionButton)

from karabo.common.api import KARABO_PROJECT_MANAGER
from karabogui import messagebox
from karabogui.binding.api import ProxyStatus
from karabogui.indicators import get_project_device_status_icon
from karabogui.request import call_device_slot
from karabogui.singletons.api import get_config, get_topology
from karabogui.util import get_reason_parts

from .utils import get_dialog_ui

HEADER_LABELS = ["Status", "deviceId", "classId", "serverId",
                 "project_name", "Instantiate"]
STATUS_COLUMN = HEADER_LABELS.index("Status")
DEVICE_ID_COLUMN = HEADER_LABELS.index("deviceId")
CLASS_ID_COLUMN = HEADER_LABELS.index("classId")
SERVER_ID_COLUMN = HEADER_LABELS.index("serverId")
INSTANTIATE_COLUMN = HEADER_LABELS.index("Instantiate")


class ButtonDelegate(QStyledItemDelegate):
    def __init__(self, parent=None):
        super().__init__(parent)

    def paint(self, painter, option, index):
        button = QStyleOptionButton()
        button.rect = option.rect
        button.text = "Instantiate"
        button.state = self.button_state(index)
        QApplication.style().drawControl(QStyle.CE_PushButton, button, painter)

    def editorEvent(self, event, model, option, index):
        if event.type() == QEvent.MouseButtonRelease:
            if option.rect.contains(event.pos()):
                self.button_clicked(index)
                return True

        return False

    def button_state(self, index):
        status = index.sibling(index.row(), STATUS_COLUMN).data(
            role=Qt.UserRole)
        state = (QStyle.State_Enabled if status == ProxyStatus.OFFLINE
                 else QStyle.State_On)
        return state

    def button_clicked(self, index):
        status = index.sibling(index.row(), STATUS_COLUMN).data(
            role=Qt.UserRole)
        if not status == ProxyStatus.OFFLINE:
            return

        def device_reply(success, reply, request):
            if not success:
                reason, details = get_reason_parts(reply)
                messagebox.show_error(reason, details=details,
                                      parent=self.parent())
                return
            else:
                deviceId = request["args.deviceId"]
                messagebox.show_information(
                    f"Device <b>{deviceId}</b> instantiated successfully.",
                    parent=self.parent())

        device_uuid = index.sibling(index.row(), INSTANTIATE_COLUMN).data(
            role=Qt.UserRole)
        deviceId = index.sibling(index.row(), DEVICE_ID_COLUMN).data(
            role=Qt.UserRole)
        class_id = index.sibling(index.row(), CLASS_ID_COLUMN).data(
            role=Qt.UserRole)
        serverId = index.sibling(index.row(), SERVER_ID_COLUMN).data(
            role=Qt.UserRole)

        call_device_slot(
            device_reply, KARABO_PROJECT_MANAGER, "slotGenericRequest",
            deviceId=deviceId,
            device_uuid=device_uuid, serverId=serverId, classId=class_id,
            type="instantiateProjectDevice")


class DeviceInstance(NamedTuple):
    status: ProxyStatus
    deviceId: str = ""
    classId: str = ""
    serverId: str = ""
    project_name: str = ""
    project_uuid: str = ""
    device_uuid: str = ""


class ProjectTopologyDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setModal(False)

        ui_file = get_dialog_ui("project_topology.ui")
        uic.loadUi(ui_file, self)

        self.delegate = ButtonDelegate()
        self.table_model = ProjectTopologyModel()
        self.filter_model = QSortFilterProxyModel()
        self.filter_model.setFilterCaseSensitivity(False)
        self.filter_model.setFilterRole(Qt.DisplayRole)
        self.filter_model.setFilterKeyColumn(DEVICE_ID_COLUMN)
        self.filter_model.setSourceModel(self.table_model)
        self.filter_text.textChanged.connect(
            self.filter_model.setFilterFixedString)
        self.clear_button.clicked.connect(self._clear_clicked)

        table_view = self.tableView
        table_view.setModel(self.filter_model)
        table_view.setWordWrap(True)
        table_view.setAlternatingRowColors(True)
        table_view.setSortingEnabled(True)
        table_view.setSelectionBehavior(QAbstractItemView.SelectRows)
        table_view.setSelectionMode(QAbstractItemView.SingleSelection)
        table_view.sortByColumn(INSTANTIATE_COLUMN, Qt.AscendingOrder)
        table_view.setItemDelegateForColumn(INSTANTIATE_COLUMN, self.delegate)
        self.table = table_view

    @Slot()
    def _clear_clicked(self):
        self.filter_text.setText("")

    def _convert_data(self, data):
        """Convert log data coming from the network"""
        topology = get_topology()

        def get_device(info):
            deviceId = info["device_name"]
            classId = info["device_class"]
            serverId = info["server_name"]
            project = info["project_name"]
            device_uuid = info["device_uuid"]
            server_attrs = topology.get_attributes(f"server.{serverId}")
            server_online = server_attrs is not None
            if not server_online:
                status = ProxyStatus.NOSERVER
            elif classId not in server_attrs.get("deviceClasses", []):
                status = ProxyStatus.NOPLUGIN
            else:
                attrs = topology.get_attributes(f"device.{deviceId}")
                status = ProxyStatus.ONLINE if attrs else ProxyStatus.OFFLINE

            return DeviceInstance(
                status=status, deviceId=deviceId, classId=classId,
                project_name=project, serverId=serverId,
                device_uuid=device_uuid)

        data = [get_device(info) for info in data]
        data = sorted(data, key=lambda d: d.deviceId)

        return data

    def initialize(self, data):
        domain = get_config()["domain"]
        self.info_label.setText(
            f"Showing the project topology from domain <b>{domain}</b>.")
        new = self._convert_data(data["items"])
        self.table_model.initialize(new)
        self._resize_table()
        self.table.sortByColumn(DEVICE_ID_COLUMN, Qt.AscendingOrder)

    def _resize_table(self):
        """Resize columns to contents"""
        columns = self.table_model.columnCount() - 1
        hor_header = self.table.horizontalHeader()
        hor_header.resizeSections(QHeaderView.ResizeToContents)
        hor_header.resizeSection(columns, QHeaderView.Stretch)
        ver_header = self.table.verticalHeader()
        ver_header.resizeSections(QHeaderView.ResizeToContents)


class ProjectTopologyModel(QAbstractTableModel):
    def __init__(self, parent=None):
        super().__init__(parent)
        self._data = []

    def headerData(self, column, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return HEADER_LABELS[column]

    def getData(self):
        return self._data

    def rowCount(self, parent=QModelIndex()):
        return len(self._data)

    def columnCount(self, parent=QModelIndex()):
        return len(HEADER_LABELS)

    def data(self, index, role=Qt.DisplayRole):
        if not index.isValid():
            return None
        device = self._data[index.row()]
        column = index.column()
        if column == 0 and role == Qt.DecorationRole:
            return get_project_device_status_icon(device.status)
        if role == Qt.DisplayRole:
            if column > 0:
                return device[index.column()]
            else:
                return device[index.column()].name
        elif role == Qt.UserRole:
            if column == INSTANTIATE_COLUMN:
                return device.device_uuid
            return device[index.column()]
        elif role == Qt.ToolTipRole:
            status = index.sibling(index.row(), STATUS_COLUMN).data(
                role=Qt.UserRole)
            return str(status)

        return None

    def initialize(self, data):
        self.beginResetModel()
        try:
            self._data = natsort.natsorted(
                data, key=lambda device: device.deviceId)
        finally:
            self.endResetModel()
