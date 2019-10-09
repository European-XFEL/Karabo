#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtGui import QVBoxLayout, QWidget

from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.navigation.system_view import SystemTreeView

from .base import BasePanelWidget
from .searchwidget import SearchBar


class TopologyPanel(BasePanelWidget):
    def __init__(self):
        super(TopologyPanel, self).__init__("System Topology")
        event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.AccessLevelChanged: self._event_access_level
        }
        register_for_broadcasts(event_map)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel."""
        widget = QWidget(self)
        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(2, 2, 2, 2)

        self.tree_view = SystemTreeView(widget)
        self.sbar = SearchBar(parent=self)
        model = self.tree_view.model()
        self.sbar.setModel(model)

        main_layout.addWidget(self.sbar)
        main_layout.addWidget(self.tree_view)

        return widget

    def _event_access_level(self, data):
        self.sbar.reset(enable=True)

    def _event_network(self, data):
        status = data.get('status', False)
        self.sbar.reset(status)
        if not status:
            self.close_popup_widget()

    def close_popup_widget(self):
        widget = self.tree_view.popupWidget
        if widget is not None:
            widget.close()
