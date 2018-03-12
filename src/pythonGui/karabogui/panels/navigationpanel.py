#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtGui import QVBoxLayout, QWidget

from karabogui.events import KaraboEventSender, register_for_broadcasts
from karabogui.navigation.view import NavigationTreeView
from .base import BasePanelWidget, Searchable


class NavigationPanel(BasePanelWidget, Searchable):
    def __init__(self):
        super(NavigationPanel, self).__init__("Navigation")
        self._init_search_filter()

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary for self due to the fact that the singleton mediator
        # object and `self` are being destroyed when the GUI exists
        register_for_broadcasts(self)

    def karaboBroadcastEvent(self, event):
        sender = event.sender
        data = event.data
        if sender is KaraboEventSender.AccessLevelChanged:
            self._init_search_filter(connected_to_server=True)
        elif sender is KaraboEventSender.NetworkConnectStatus:
            self._init_search_filter(data.get('status', False))
        return False

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)
        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(5, 5, 5, 5)

        self.twNavigation = NavigationTreeView(widget)
        h_layout = self.create_search_bar(self.twNavigation.model())
        main_layout.addLayout(h_layout)

        main_layout.addWidget(self.twNavigation)
        return widget
