#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from karabo_gui.events import register_for_broadcasts, KaraboEventSender
from karabo_gui.logwidget import LogWidget
from .base import BasePanelWidget


class NotificationPanel(BasePanelWidget):
    def __init__(self):
        super(NotificationPanel, self).__init__("Notifications")

        # Register for broadcast events.
        # This object lives as long as the app. No need to unregister.
        register_for_broadcasts(self)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        self.logWidget = LogWidget(self, False)
        return self.logWidget

    def karaboBroadcastEvent(self, event):
        """ Router for incoming broadcasts
        """
        if event.sender is KaraboEventSender.NotificationMessage:
            device_id = event.data['device_id']
            message_type = event.data['message_type']
            short_msg = event.data['short_msg']
            detailed_msg = event.data['detailed_msg']
            handler = self.logWidget.onNotificationAvailable
            handler(device_id, message_type, short_msg, detailed_msg)
        return False
