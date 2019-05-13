#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QVBoxLayout, QWidget

from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.navigation.system_view import SystemTreeView
from karabogui.util import wait_cursor
from .base import BasePanelWidget


class TopologyPanel(BasePanelWidget):
    def __init__(self):
        super(TopologyPanel, self).__init__("System Topology")
        event_map = {
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
        self.tool_widget.ui_clear_button.clicked.connect(
            self._clear_clicked)
        self.tool_widget.ui_search_filter.returnPressed.connect(
            self._search_clicked)

        main_layout.addWidget(self.tool_widget)
        main_layout.addWidget(self.tree_view)

        return widget

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
        with wait_cursor():
            pattern = str(self.tool_widget.ui_search_filter.text())
            proxy_model = self.tree_view.model()
            proxy_model.setFilterFixedString(pattern)
            self.tree_view.expandAll()

    @pyqtSlot()
    def _clear_clicked(self):
        with wait_cursor():
            pattern = ''
            self.tool_widget.ui_search_filter.setText(pattern)
            proxy_model = self.tree_view.model()
            proxy_model.setFilterFixedString(pattern)
            self.tree_view.expandAll()
            # After search, the operator can clear and maintain his selection!
            index = proxy_model.currentIndex()
            if index.isValid():
                self.tree_view.scrollTo(index)
