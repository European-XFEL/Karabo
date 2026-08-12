#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on August 12, 2026
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

from itertools import zip_longest

from qtpy import uic
from qtpy.QtCore import QSortFilterProxyModel, Qt, Slot
from qtpy.QtGui import QPalette, QStandardItem, QStandardItemModel
from qtpy.QtWidgets import (
    QAbstractItemView, QComboBox, QDialog, QHeaderView, QLineEdit, QListView,
    QSplitter, QStackedWidget, QTableView, QTextEdit, QTreeView, QVBoxLayout,
    QWidget)

from karabo.native import Hash, create_html_hash
from karabogui import icons
from karabogui.events import (
    KaraboEvent, register_for_broadcasts, unregister_from_broadcasts)
from karabogui.singletons.api import get_network
from karabogui.util import get_spin_widget

from .utils import get_dialog_ui

BLANK_PAGE = 0
WAITING_PAGE = 1
INFO_PAGE = 2


class GuiDebugInfo(QDialog):
    def __init__(self, parent: QWidget | None = None):
        super().__init__(parent=parent)
        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setModal(False)
        uic.loadUi(get_dialog_ui("guidebuginfo.ui"), self)
        self.set_stack_index(BLANK_PAGE)

        self.gui_info: Hash | None = None

        # Stacked widget parameters, blank
        self.stacked_widget.addWidget(QTextEdit(self))

        wait_widget = get_spin_widget(icon="wait", parent=self)
        wait_widget.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        wait_widget.setAutoFillBackground(True)
        wait_widget.setBackgroundRole(QPalette.Base)
        self.stacked_widget.addWidget(wait_widget)

        splitter = QSplitter(Qt.Horizontal, self)
        self.info_widget = QTextEdit(splitter)
        self.info_widget.setReadOnly(True)

        details_panel = QWidget(splitter)
        details_layout = QVBoxLayout(details_panel)
        details_layout.setContentsMargins(0, 0, 0, 0)
        self.details_selector = QComboBox(details_panel)
        self.details_selector.addItems([
            "Topology", "Monitored Devices", "Pipeline Connections"])
        self.details_stack = QStackedWidget(details_panel)
        self.details_selector.currentIndexChanged.connect(
            self.details_stack.setCurrentIndex)

        topology_panel = QWidget(self.details_stack)
        topology_layout = QVBoxLayout(topology_panel)
        topology_layout.setContentsMargins(0, 0, 0, 0)
        self.search_field = QLineEdit(topology_panel)
        self.search_field.setPlaceholderText("Search server or device")
        self.search_field.textChanged.connect(self._filter_topology)
        self.topology = QTreeView(topology_panel)
        self.topology.setAlternatingRowColors(True)
        self.topology.setSelectionMode(QAbstractItemView.NoSelection)
        self.topology.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self.model = QStandardItemModel(parent=self.topology)
        self.model.setHorizontalHeaderLabels(["Instance Type", "Instance Id"])
        self.filter_model = QSortFilterProxyModel(parent=self.topology)
        self.filter_model.setSourceModel(self.model)
        self.filter_model.setFilterCaseSensitivity(Qt.CaseInsensitive)
        # Note: -1 is all columns
        self.filter_model.setFilterKeyColumn(-1)
        self.filter_model.setRecursiveFilteringEnabled(True)
        self.filter_model.setAutoAcceptChildRows(True)

        self.topology.setModel(self.filter_model)
        header = self.topology.header()
        header.setSectionResizeMode(0, QHeaderView.ResizeToContents)
        header.setSectionResizeMode(1, QHeaderView.Stretch)
        header.sectionDoubleClicked.connect(self._toggle_topology)
        topology_layout.addWidget(self.search_field)
        topology_layout.addWidget(self.topology)

        monitored_panel = QWidget(self.details_stack)
        monitored_layout = QVBoxLayout(monitored_panel)
        monitored_layout.setContentsMargins(0, 0, 0, 0)
        self.monitored_search = QLineEdit(monitored_panel)
        self.monitored_search.setPlaceholderText("Search monitored devices")
        self.monitored_search.textChanged.connect(self._filter_monitored)
        self.monitored_view = QListView(monitored_panel)
        self.monitored_view.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self.monitored_model = QStandardItemModel(parent=self.monitored_view)
        self.monitored_filter_model = QSortFilterProxyModel(
            parent=self.monitored_view)
        self.monitored_filter_model.setSourceModel(self.monitored_model)
        self.monitored_filter_model.setFilterCaseSensitivity(
            Qt.CaseInsensitive)
        self.monitored_view.setModel(self.monitored_filter_model)
        monitored_layout.addWidget(self.monitored_search)
        monitored_layout.addWidget(self.monitored_view)

        pipeline_panel = QWidget(self.details_stack)
        pipeline_layout = QVBoxLayout(pipeline_panel)
        pipeline_layout.setContentsMargins(0, 0, 0, 0)
        self.pipeline_table = QTableView(pipeline_panel)
        self.pipeline_table.setEditTriggers(QAbstractItemView.NoEditTriggers)
        self.pipeline_model = QStandardItemModel(parent=self.pipeline_table)
        self.pipeline_model.setHorizontalHeaderLabels(
            ["Pipeline Connection", "Ready"])
        self.pipeline_table.setModel(self.pipeline_model)
        pipeline_header = self.pipeline_table.horizontalHeader()
        pipeline_header.setSectionResizeMode(0, QHeaderView.Stretch)
        pipeline_header.setSectionResizeMode(1, QHeaderView.ResizeToContents)
        pipeline_layout.addWidget(self.pipeline_table)

        self.details_stack.addWidget(topology_panel)
        self.details_stack.addWidget(monitored_panel)
        self.details_stack.addWidget(pipeline_panel)
        details_layout.addWidget(self.details_selector)
        details_layout.addWidget(self.details_stack)

        splitter.addWidget(self.info_widget)
        splitter.addWidget(details_panel)
        splitter.setStretchFactor(0, 1)
        splitter.setStretchFactor(1, 2)
        self.stacked_widget.addWidget(splitter)

        self.refresh_button.setIcon(icons.refresh)
        self.refresh_button.clicked.connect(self.refresh)

        self._event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.ClientDebugInfo: self._event_info}
        register_for_broadcasts(self._event_map)
        self.refresh()
        self.expanded = True

    def set_stack_index(self, index):
        self.stacked_widget.blockSignals(True)
        self.stacked_widget.setCurrentIndex(index)
        self.stacked_widget.blockSignals(False)

    @Slot()
    def refresh(self):
        self.gui_info = None
        self.set_stack_index(WAITING_PAGE)
        get_network().getClientDebugInfo()

    def done(self, result):
        unregister_from_broadcasts(self._event_map)
        return super().done(result)

    def _event_network(self, data):
        if not data.get("status"):
            self.close()

    def _event_info(self, data):
        info = data.get("info")
        self.gui_info = info
        topology = info.pop("systemTopology", Hash())

        # first is channel address
        client_key = next(iter(info), None)
        client_info = (info.get(client_key, Hash())
                       if client_key is not None else Hash())
        # Extract client information for widgets
        monitoredDevices: list[str] = client_info.get(
            "monitoredDevices", [])
        pipelineConnections: list[str] = client_info.get(
            "pipelineConnections", [])
        pipelineConnectionsReadiness: list[bool] = client_info.get(
            "pipelineConnectionsReadiness", [])

        self.info_widget.setHtml(create_html_hash(info))
        self._set_topology(topology)
        self._set_monitored_devices(monitoredDevices)
        self._set_pipeline_connections(
            pipelineConnections, pipelineConnectionsReadiness)
        self.set_stack_index(INFO_PAGE)

    def _set_monitored_devices(self, monitored_devices):
        self.monitored_model.removeRows(0, self.monitored_model.rowCount())
        for device_id in monitored_devices:
            item = QStandardItem(str(device_id))
            item.setEditable(False)
            self.monitored_model.appendRow(item)

    def _set_pipeline_connections(self, connections, readiness):
        self.pipeline_model.removeRows(0, self.pipeline_model.rowCount())
        for connection, ready in zip_longest(connections, readiness,
                                             fillvalue=""):
            connection_item = QStandardItem(str(connection))
            readiness_item = QStandardItem(str(ready))
            connection_item.setEditable(False)
            readiness_item.setEditable(False)
            self.pipeline_model.appendRow([connection_item, readiness_item])

    @Slot(str)
    def _filter_monitored(self, text):
        self.monitored_filter_model.setFilterFixedString(text)

    def _set_topology(self, topology: Hash):
        """Populate the topology tree from the GuiServer debug information."""
        self.model.removeRows(0, self.model.rowCount())
        root = self.model.invisibleRootItem()
        for instance_type, instances in topology.items():
            type_item = QStandardItem(str(instance_type))
            font = type_item.font()
            font.setBold(True)
            type_item.setFont(font)
            type_item.setEditable(False)
            id_item = QStandardItem()
            id_item.setEditable(False)
            root.appendRow([type_item, id_item])
            self._add_instances(type_item, instances, instance_type)
        self.topology.expandToDepth(1)

    @Slot(str)
    def _filter_topology(self, text):
        self.filter_model.setFilterFixedString(text)
        if text:
            self.topology.expandAll()
        else:
            self.topology.expandToDepth(1)

    @Slot()
    def _toggle_topology(self):
        if self.expanded:
            self.topology.collapseAll()
        else:
            self.topology.expandAll()
        self.expanded = not self.expanded

    def _add_instances(
            self, parent: QStandardItem, hsh: Hash, instance_type: str):
        for instanceId, _, _ in hsh.iterall():
            spacer = QStandardItem()
            spacer.setEditable(False)
            id_item = QStandardItem(instanceId)
            id_item.setEditable(False)
            if instance_type in {"device", "macro", "client"}:
                id_item.setIcon(icons.deviceInstance)
            elif instance_type == "unknown":
                id_item.setIcon(icons.deviceIncompatible)
            elif instance_type == "server":
                id_item.setIcon(icons.yes)
            parent.appendRow([spacer, id_item])
