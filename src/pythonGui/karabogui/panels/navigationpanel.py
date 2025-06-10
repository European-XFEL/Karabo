#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
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

from qtpy.QtCore import Slot
from qtpy.QtWidgets import QPushButton, QVBoxLayout, QWidget

from karabo.common.api import (
    KARABO_DAEMON_MANAGER, KARABO_SCHEMA_DEFAULT_SCENE)
from karabogui import messagebox
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.navigation.system_view import SystemTreeView
from karabogui.request import get_scene_from_server
from karabogui.singletons.api import get_config
from karabogui.topology.api import is_device_online

from .base import BasePanelWidget
from .tool_widget import SearchBar


class TopologyPanel(BasePanelWidget):
    def __init__(self):
        super().__init__("System Topology")
        event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.AccessLevelChanged: self._event_access_level,
            KaraboEvent.ShowDevice: self._event_show_device
        }
        register_for_broadcasts(event_map)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel."""
        widget = QWidget(self)
        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(2, 2, 2, 2)

        self.tree_view = SystemTreeView(widget)
        self.sbar = SearchBar(parent=widget)
        self.daemon_button = QPushButton("Service Manager", parent=widget)
        self.daemon_button.clicked.connect(self._retrieve_service_scene)
        self.daemon_button.setVisible(True)
        self.sbar.setView(self.tree_view)

        main_layout.addWidget(self.sbar)
        main_layout.addWidget(self.daemon_button)
        main_layout.addWidget(self.tree_view)

        return widget

    def __repr__(self):
        topic = get_config()["broker_topic"]
        return f"<TopologyPanel topic={topic}>"

    # -----------------------------------------------------------------------
    # Karabo Events

    def _event_access_level(self, data):
        self.sbar.reset(enable=True)

    def _event_network(self, data):
        status = data.get('status', False)
        self.sbar.reset(status)
        self.daemon_button.setVisible(status)
        if not status:
            self.close_popup_widget()

    def _event_show_device(self, data):
        if data.get('showTopology'):
            self.panel_container.switch_to_panel(self)

    # -----------------------------------------------------------------------

    def close_popup_widget(self):
        widget = self.tree_view.popupWidget
        if widget is not None:
            widget.close()

    @Slot()
    def _retrieve_service_scene(self):
        instance_id = KARABO_DAEMON_MANAGER
        if not is_device_online(instance_id):
            messagebox.show_error(
                "The `KaraboDaemonManager` device is not online.",
                parent=self)
            return
        get_scene_from_server(instance_id, KARABO_SCHEMA_DEFAULT_SCENE)
