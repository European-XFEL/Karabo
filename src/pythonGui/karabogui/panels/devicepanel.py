#############################################################################
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtGui import QVBoxLayout, QWidget

from karabogui.events import KaraboEventSender, register_for_broadcasts
from karabogui.navigation.device_view import DeviceTreeView
from .base import BasePanelWidget


class DevicePanel(BasePanelWidget):
    def __init__(self):
        super(DevicePanel, self).__init__("Device Topology")
        # We need broadcasts!
        register_for_broadcasts(self)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)
        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(5, 5, 5, 5)
        self.tree_view = DeviceTreeView(widget)

        main_layout.addWidget(self.tree_view)
        return widget

    def karaboBroadcastEvent(self, event):
        sender = event.sender
        data = event.data
        if sender is KaraboEventSender.NetworkConnectStatus:
            status = data.get('status', False)
            if not status:
                self.close_popup_widget()

        return False

    def close_popup_widget(self):
        widget = self.tree_view.popupWidget
        if widget is not None:
            widget.close()
