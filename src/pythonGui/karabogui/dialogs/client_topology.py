#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on May 17, 2021
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy import uic
from qtpy.QtCore import QPoint, QSize, Qt, Slot
from qtpy.QtGui import QStandardItem, QStandardItemModel
from qtpy.QtWidgets import (
    QAbstractItemView, QDialog, QHeaderView, QMenu, QTreeView)

from karabo.common.api import ProxyStatus
from karabogui import icons
from karabogui.singletons.api import get_manager, get_topology

from .utils import get_dialog_ui

HEADER_LABELS = ["Client Id", "Host"]


class ClientTopologyTreeView(QTreeView):
    def __init__(self, parent):
        super().__init__(parent)
        model = QStandardItemModel(parent=self)
        model.setHorizontalHeaderLabels(HEADER_LABELS)
        self.setModel(model)
        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)

        header = self.header()
        header.setSectionResizeMode(QHeaderView.ResizeToContents)
        self.popupWidget = None
        self.setUniformRowHeights(True)

    @Slot()
    def refresh(self):
        model = self.model()
        model.clear()
        model.setHorizontalHeaderLabels(HEADER_LABELS)

        system_hash = get_topology()._system_hash
        if 'client' not in system_hash:
            return

        root_item = model.invisibleRootItem()
        for name, _, attrs in system_hash['client'].iterall():
            status = ProxyStatus(attrs.get('status', 'ok'))
            client_item = QStandardItem(name)
            if status is ProxyStatus.ERROR:
                icon = icons.deviceInstanceError
            else:
                icon = icons.deviceInstance
            client_item.setIcon(icon)
            client_item.setEditable(False)

            host_item = QStandardItem(attrs.get('host', 'None'))
            host_item.setEditable(False)
            root_item.appendRow([client_item, host_item])


class ClientTopologyDialog(QDialog):
    def __init__(self, parent):
        super().__init__(parent=parent)
        self.setModal(False)

        ui_file = get_dialog_ui("client_topology.ui")
        uic.loadUi(ui_file, self)

        self.tree_view = ClientTopologyTreeView(self)
        self.ui_refresh.setIcon(icons.refresh)
        self.ui_refresh.clicked.connect(self.tree_view.refresh)

        layout = self.ui_frame
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.tree_view)

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self._context_menu)

        # Refresh at start!
        self.tree_view.refresh()

    @Slot(QPoint)
    def _context_menu(self, pos):
        selection_model = self.tree_view.selectionModel()
        index = selection_model.currentIndex()
        if not index.isValid():
            return

        menu = QMenu(parent=self)
        shutdown_action = menu.addAction('Shutdown client')
        shutdown_action.triggered.connect(self.onKillInstance)
        shutdown_action.setIcon(icons.delete)

        menu.exec_(self.mapToGlobal(pos))

    @Slot()
    def onKillInstance(self):
        index = self.tree_view.selectionModel().currentIndex()
        deviceId = index.siblingAtColumn(0).data()
        manager = get_manager()
        manager.shutdownDevice(deviceId, parent=self)

    def sizeHint(self):
        return QSize(640, 480)