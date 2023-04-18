#############################################################################
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################

from qtpy.QtWidgets import QVBoxLayout, QWidget

from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.navigation.device_view import DeviceTreeView

from .base import BasePanelWidget
from .tool_widget import InterfaceBar


class DevicePanel(BasePanelWidget):
    def __init__(self):
        super(DevicePanel, self).__init__("Device Topology")
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
