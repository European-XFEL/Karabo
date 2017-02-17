#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from karabo_gui.events import (
    register_for_broadcasts, KaraboBroadcastEvent, KaraboEventSender)
from karabo_gui.navigationtreeview import NavigationTreeView
from .base import BasePanelWidget


class NavigationPanel(BasePanelWidget):
    def __init__(self):
        super(NavigationPanel, self).__init__("Navigation")

        # Register for broadcast events.
        # This object lives as long as the app. No need to unregister.
        register_for_broadcasts(self)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        self.twNavigation = NavigationTreeView(self)
        return self.twNavigation

    def eventFilter(self, obj, event):
        """ Router for incoming broadcasts
        """
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.NetworkConnectStatus:
                if not event.data['status']:
                    self.twNavigation.clear()
            return False
        return super(NavigationPanel, self).eventFilter(obj, event)

    def onSelectNewNavigationItem(self, devicePath):
        self.twNavigation.selectItem(devicePath)

    def onGlobalAccessLevelChanged(self):
        self.twNavigation.model().globalAccessLevelChanged()
