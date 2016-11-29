#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QVBoxLayout, QWidget

from karabo_gui.docktabwindow import Dockable
from karabo_gui.navigationtreeview import NavigationTreeView
from karabo_gui.mediator import (
    register_for_broadcasts, KaraboBroadcastEvent, KaraboEventSender)


class NavigationPanel(Dockable, QWidget):
    def __init__(self):
        super(NavigationPanel, self).__init__()

        # Register for broadcast events.
        # This object lives as long as the app. No need to unregister.
        register_for_broadcasts(self)

        title = "Navigation"
        self.setWindowTitle(title)

        self.twNavigation = NavigationTreeView(self)

        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5, 5, 5, 5)
        mainLayout.addWidget(self.twNavigation)

    def eventFilter(self, obj, event):
        """ Router for incoming broadcasts
        """
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.NetworkDisconnected:
                # The last selection is not needed anymore
                self.twNavigation.clear()
            return False
        return super(NavigationPanel, self).eventFilter(obj, event)

    def onSelectNewNavigationItem(self, devicePath):
        self.twNavigation.selectItem(devicePath)

    def onGlobalAccessLevelChanged(self):
        self.twNavigation.model().globalAccessLevelChanged()
