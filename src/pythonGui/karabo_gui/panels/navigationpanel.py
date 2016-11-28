#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtGui import QVBoxLayout, QWidget

from karabo_gui.docktabwindow import Dockable
from karabo_gui.navigationtreeview import NavigationTreeView
from karabo_gui.singletons.api import get_manager


class NavigationPanel(Dockable, QWidget):
    def __init__(self):
        super(NavigationPanel, self).__init__()

        title = "Navigation"
        self.setWindowTitle(title)

        self.twNavigation = NavigationTreeView(self)

        get_manager().signalReset.connect(self.onResetPanel)

        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5, 5, 5, 5)
        mainLayout.addWidget(self.twNavigation)

    def onResetPanel(self):
        """
        This slot is called when the panel needs a reset which means the last
        selection is not needed anymore.
        """
        self.twNavigation.clear()

    def onSelectNewNavigationItem(self, devicePath):
        self.twNavigation.selectItem(devicePath)

    def onGlobalAccessLevelChanged(self):
        self.twNavigation.model().globalAccessLevelChanged()
