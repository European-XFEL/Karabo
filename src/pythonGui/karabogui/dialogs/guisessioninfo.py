#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on November 13, 2024
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

from qtpy import uic
from qtpy.QtCore import Qt, Slot
from qtpy.QtGui import QPalette
from qtpy.QtWidgets import (
    QDialog, QHeaderView, QListWidgetItem, QTableWidget, QTableWidgetItem,
    QWidget)

from karabo.native import Hash, Timestamp
from karabogui import icons
from karabogui.events import (
    KaraboEvent, register_for_broadcasts, unregister_from_broadcasts)
from karabogui.request import call_device_slot
from karabogui.topology.api import getTopology
from karabogui.util import get_spin_widget

from .utils import get_dialog_ui

BLANK_PAGE = 0
WAITING_PAGE = 1
INFO_PAGE = 2


class GuiSessionInfo(QDialog):
    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent=parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        uic.loadUi(get_dialog_ui("guiserverinfo.ui"), self)

        self.set_stack_index(BLANK_PAGE)

        # Stacked widget parameters
        self.stacked_widget.addWidget(QTableWidget(self))

        wait_widget = get_spin_widget(icon="wait", parent=self)
        wait_widget.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        wait_widget.setAutoFillBackground(True)
        wait_widget.setBackgroundRole(QPalette.Base)
        self.stacked_widget.addWidget(wait_widget)

        self.table_widget = QTableWidget(self)
        self.stacked_widget.addWidget(self.table_widget)

        self.fill_servers()
        self.list_widget.itemClicked.connect(self.itemClicked)
        self.table_widget.setColumnCount(3)
        self.table_widget.setHorizontalHeaderLabels(
            ["Version", "Session start", "Session token"])

        header = self.table_widget.horizontalHeader()
        header.setSectionResizeMode(0, QHeaderView.ResizeToContents)
        header.setSectionResizeMode(1, QHeaderView.ResizeToContents)
        header.setSectionResizeMode(2, QHeaderView.Stretch)

        self.refresh_button.setIcon(icons.refresh)
        self.refresh_button.clicked.connect(self.refresh)

        self._event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network}
        register_for_broadcasts(self._event_map)

    def fill_servers(self):
        topology = getTopology()
        devices = []
        for deviceId, _, attrs in topology.get("device", Hash()).iterall():
            if attrs.get("classId") == "GuiServerDevice":
                devices.append(deviceId)
        if devices:
            self.list_widget.addItems(devices)
            self.list_widget.setCurrentRow(0)
            self.itemClicked(self.list_widget.currentItem())

    def set_stack_index(self, index):
        self.stacked_widget.blockSignals(True)
        self.stacked_widget.setCurrentIndex(index)
        self.stacked_widget.blockSignals(False)

    @Slot()
    def refresh(self):
        item = self.list_widget.currentItem()
        if item is not None:
            self.itemClicked(item)

    @Slot(QListWidgetItem)
    def itemClicked(self, item):

        def handler(success, reply):
            if not success:
                self.set_stack_index(BLANK_PAGE)
                return

            self.set_stack_index(INFO_PAGE)
            sessions = reply["clientSessions"]
            self.table_widget.setRowCount(len(sessions))
            for row, session in enumerate(sessions):
                item = QTableWidgetItem(session["clientVersion"])
                item.setFlags(item.flags() & ~Qt.ItemIsEditable)
                self.table_widget.setItem(row, 0, item)

                timestamp = Timestamp(
                    session["sessionStartTime"]).toLocal(" ")
                item = QTableWidgetItem(timestamp)
                item.setFlags(item.flags() & ~Qt.ItemIsEditable)
                self.table_widget.setItem(row, 1, item)

                item = QTableWidgetItem(session["sessionToken"])
                item.setFlags(item.flags() & ~Qt.ItemIsEditable)
                self.table_widget.setItem(row, 2, item)

        self.set_stack_index(WAITING_PAGE)
        deviceId = item.data(Qt.DisplayRole)
        call_device_slot(handler, deviceId, "slotGetClientSessions")

    def done(self, result):
        unregister_from_broadcasts(self._event_map)
        return super().done(result)

    def _event_network(self, data):
        if not data.get("status"):
            self.close()
