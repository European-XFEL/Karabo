#############################################################################
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtGui import QVBoxLayout, QWidget

from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.navigation.device_view import DeviceTreeView
from .base import BasePanelWidget


class DevicePanel(BasePanelWidget):
    def __init__(self):
        super(DevicePanel, self).__init__("Device Topology")
        # We need broadcasts!
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
        self.tree_view = DeviceTreeView(widget)

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
