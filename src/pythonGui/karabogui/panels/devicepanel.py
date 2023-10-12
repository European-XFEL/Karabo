#############################################################################
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

from qtpy.QtWidgets import QVBoxLayout, QWidget

from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.navigation.device_view import DeviceTreeView
from karabogui.singletons.api import get_config

from .base import BasePanelWidget
from .tool_widget import InterfaceBar


class DevicePanel(BasePanelWidget):
    def __init__(self):
        super().__init__("Device Topology")
        # We need broadcasts!
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
        main_layout.setContentsMargins(2, 2, 2, 2)
        self.tree_view = DeviceTreeView(widget)

        self.sbar = InterfaceBar(parent=widget)
        self.sbar.setView(self.tree_view)

        main_layout.addWidget(self.sbar)
        main_layout.addWidget(self.tree_view)

        return widget

    def _event_access_level(self, data):
        self.sbar.reset(enable=True)

    def _event_network(self, data):
        status = data.get('status', False)
        if not status:
            self.close_popup_widget()
        self.sbar.reset(status)

    def close_popup_widget(self):
        widget = self.tree_view.popupWidget
        if widget is not None:
            widget.close()

    def __repr__(self):
        topic = get_config()["broker_topic"]
        return f"<DevicePanel topic={topic}>"
