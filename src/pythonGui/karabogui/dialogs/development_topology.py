# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

from qtpy import uic
from qtpy.QtCore import QPoint, QSize, Qt, Slot
from qtpy.QtGui import QStandardItem, QStandardItemModel
from qtpy.QtWidgets import (
    QAbstractItemView, QDialog, QHeaderView, QMenu, QTreeView)

from karabo.common.api import ServerFlags
from karabogui import icons
from karabogui.dialogs.utils import get_dialog_ui
from karabogui.navigation.utils import get_language_icon
from karabogui.singletons.api import get_manager, get_topology

HEADER_LABELS = ["Server Id", "Host"]


class DevelopmentTopologyView(QTreeView):
    def __init__(self, parent=None):
        super().__init__(parent)
        model = QStandardItemModel(parent=self)
        model.setHorizontalHeaderLabels(HEADER_LABELS)
        self.setModel(model)
        self.setSelectionMode(QAbstractItemView.SingleSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)

        header = self.header()
        header.setSectionResizeMode(QHeaderView.ResizeToContents)
        self.setUniformRowHeights(True)

    @Slot()
    def refresh(self):
        model = self.model()
        model.clear()
        model.setHorizontalHeaderLabels(HEADER_LABELS)

        topology = get_topology()
        root_item = model.invisibleRootItem()

        def visitor(node):
            attributes = node.attributes

            if (attributes.get("serverFlags", 0) & ServerFlags.Development
                    == ServerFlags.Development):
                icon = get_language_icon(node)
                server_item = QStandardItem(icon, node.path)
                server_item.setEditable(False)

                host = attributes.get("host", "None")
                host_item = QStandardItem(host)
                host_item.setEditable(False)

                root_item.appendRow([server_item, host_item])

        topology.visit_system_tree(visitor)


class DevelopmentTopologyDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        ui_file = get_dialog_ui("development_topology.ui")
        uic.loadUi(ui_file, self)

        self.tree_view = DevelopmentTopologyView(parent=self)

        self.ui_refresh.setIcon(icons.refresh)
        self.ui_refresh.clicked.connect(self.tree_view.refresh)

        layout = self.ui_frame
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.tree_view)

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self._context_menu)

        self.tree_view.refresh()

    @Slot(QPoint)
    def _context_menu(self, pos):
        index = self.tree_view.selectionModel().currentIndex()
        if not index.isValid():
            return

        menu = QMenu(parent=self)
        shutdown_action = menu.addAction('Shutdown')
        shutdown_action.triggered.connect(self.onKillInstance)
        shutdown_action.setIcon(icons.delete)

        menu.exec(self.mapToGlobal(pos))

    @Slot()
    def onKillInstance(self):
        manager = get_manager()
        index = self.tree_view.selectionModel().currentIndex()
        serverId = index.sibling(index.row(), 0).data()
        manager.shutdownServer(serverId, parent=self)

    def sizeHint(self):
        return QSize(640, 480)
