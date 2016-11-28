#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 1, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtGui import QVBoxLayout, QWidget

from karabo_gui.docktabwindow import Dockable
from karabo_gui.logwidget import LogWidget
from karabo_gui.mediator import (
    register_for_broadcasts, KaraboBroadcastEvent, KaraboEventSender)


class NotificationPanel(Dockable, QWidget):
    def __init__(self):
        super(NotificationPanel, self).__init__()

        self.__logWidget = LogWidget(self, False)

        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5, 5, 5, 5)
        mainLayout.addWidget(self.__logWidget)

        register_for_broadcasts(self)

    def eventFilter(self, obj, event):
        """ Router for incoming broadcasts
        """
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.NotificationMessage:
                device_id = event.data['device_id']
                message_type = event.data['message_type']
                short_msg = event.data['short_msg']
                detailed_msg = event.data['detailed_msg']
                handler = self.__logWidget.onNotificationAvailable
                handler(device_id, message_type, short_msg, detailed_msg)
            return False
        return super(NotificationPanel, self).eventFilter(obj, event)
