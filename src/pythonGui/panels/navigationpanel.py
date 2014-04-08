#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the navigation panel on the
   left of the MainWindow which is un/dockable.
"""

__all__ = ["NavigationPanel"]


import const

from enums import NavigationItemTypes
from manager import Manager
from navigationtreeview import NavigationTreeView

from PyQt4.QtCore import pyqtSignal, SIGNAL
from PyQt4.QtGui import QVBoxLayout, QWidget

class NavigationPanel(QWidget):
    ##########################################
    # Dockable widget class used in DivWidget
    # Requires following interface:
    #
    #def setupActions(self):
    #    pass
    #def setupToolBars(self, standardToolBar, parent):
    #    pass
    #def onUndock(self):
    #    pass
    #def onDock(self):
    #    pass
    ##########################################


    def __init__(self):
        super(NavigationPanel, self).__init__()
        
        title = "Navigation"
        self.setWindowTitle(title)
                
        self.__twNavigation = NavigationTreeView(self)
        self.__twNavigation.model().signalInstanceNewReset.connect(self.onResetPanel)

        Manager().signalGlobalAccessLevelChanged.connect(self.onGlobalAccessLevelChanged)
        
        Manager().signalReset.connect(self.onResetPanel)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.__twNavigation)

        self.setupActions()


### getter functions ###
    def _navigationTreeView(self):
        return self.__twNavigation
    navigationTreeView = property(fget=_navigationTreeView)


### initializations ###
    def setupActions(self):
        pass


    def setupToolBars(self, toolBar, parent):
        pass


### slots ###
    def onResetPanel(self):
        """
        This slot is called when the panel needs a reset which means the last
        selection is not needed anymore.
        """
        self.__twNavigation.clearSelection()


    def onNewNavigationItem(self, itemInfo):
        # itemInfo: id, name, type, (status), (refType), (refId), (schema)
        self.__twNavigation.createNewItem(itemInfo, True)


    def onSelectNewNavigationItem(self, devicePath):
        self.__twNavigation.selectItem(devicePath)

   
    def onGlobalAccessLevelChanged(self):
        self.__twNavigation.model().globalAccessLevelChanged()


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass

