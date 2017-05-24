#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from karabo_gui.navigationtreeview import NavigationTreeView
from .base import BasePanelWidget


class NavigationPanel(BasePanelWidget):
    def __init__(self):
        super(NavigationPanel, self).__init__("Navigation")

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        self.twNavigation = NavigationTreeView(self)
        return self.twNavigation
