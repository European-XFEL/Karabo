#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the navigation panel on the
   left of the MainWindow which is un/dockable.
"""

__all__ = ["NavigationPanel"]


from docktabwindow import Dockable
from manager import Manager
from navigationtreeview import NavigationTreeView

from PyQt4.QtGui import QVBoxLayout, QWidget

class NavigationPanel(Dockable, QWidget):
    def __init__(self):
        super(NavigationPanel, self).__init__()
        
        title = "Navigation"
        self.setWindowTitle(title)
                
        self.twNavigation = NavigationTreeView(self)
        
        Manager().signalReset.connect(self.onResetPanel)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.twNavigation)

    def onResetPanel(self):
        """
        This slot is called when the panel needs a reset which means the last
        selection is not needed anymore.
        """
        self.twNavigation.clear()


    def onNewNavigationItem(self, itemInfo):
        # itemInfo: id, name, type, (status), (refType), (refId), (schema)
        self.twNavigation.createNewItem(itemInfo, True)


    def onSelectNewNavigationItem(self, devicePath):
        self.twNavigation.selectItem(devicePath)

   
    def onGlobalAccessLevelChanged(self):
        self.twNavigation.model().globalAccessLevelChanged()
