#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot, QRegExp, Qt
from PyQt4.QtGui import QVBoxLayout, QWidget

from karabogui import icons
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.navigation.system_view import SystemTreeView

from .base import BasePanelWidget


class TopologyPanel(BasePanelWidget):
    def __init__(self):
        super(TopologyPanel, self).__init__("System Topology")
        event_map = {
            KaraboEvent.AccessLevelChanged: self._event_access_level,
            KaraboEvent.NetworkConnectStatus: self._event_network
        }
        register_for_broadcasts(event_map)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)
        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(5, 5, 5, 5)

        self.tree_view = SystemTreeView(widget)
        self.tool_widget = self._create_tool_widget()
        self.tool_widget.ui_search_button.clicked.connect(
            self._search_clicked)
        self.tool_widget.ui_collapse.setIcon(icons.collapse)
        self.tool_widget.ui_expand.setIcon(icons.expand)
        self.tool_widget.ui_collapse.clicked.connect(
            self._collapse_clicked)
        self.tool_widget.ui_expand.clicked.connect(
            self._expand_clicked)

        main_layout.addWidget(self.tool_widget)
        main_layout.addWidget(self.tree_view)

        return widget

    def _event_access_level(self, data):
        pass

    def _event_network(self, data):
        status = data.get('status', False)
        if not status:
            self.close_popup_widget()

    def close_popup_widget(self):
        widget = self.tree_view.popupWidget
        if widget is not None:
            widget.close()

    def _create_tool_widget(self):
        tool_widget = QWidget()
        uic.loadUi(op.join(op.dirname(__file__), "tool_widget.ui"),
                   tool_widget)
        return tool_widget

    # ---------------------------------------------------------------------
    # Qt Slots

    @pyqtSlot()
    def _search_clicked(self):
        pattern = str(self.tool_widget.ui_search_filter.text())
        proxy_model = self.tree_view.model()
        proxy_model.invalidateFilter()
        case_sensitive = self.tool_widget.ui_case_sensitive.isChecked()
        cs = Qt.CaseSensitive if case_sensitive else Qt.CaseInsensitive
        if self.tool_widget.ui_full_match.isChecked():
            pattern = '^{}$'.format(pattern)
        proxy_model.setFilterRegExp(QRegExp(pattern, cs, QRegExp.RegExp))
        proxy_model.selectionModel.clearSelection()

        self.tree_view.expandAll()

    @pyqtSlot()
    def _expand_clicked(self):
        """Expand the tree in the navigation panel
        """
        self.tree_view.expandAll()

    @pyqtSlot()
    def _collapse_clicked(self):
        """Expand the tree in the navigation panel
        """
        self.tree_view.collapseAll()
