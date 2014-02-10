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

    def __init__(self, treemodel):
        super(NavigationPanel, self).__init__()
        
        title = "Navigation"
        self.setWindowTitle(title)
                
        self.__twNavigation = NavigationTreeView(self, treemodel)

        Manager().signalGlobalAccessLevelChanged.connect(self.onGlobalAccessLevelChanged)
        
        Manager().signalNewNavigationItem.connect(self.onNewNavigationItem)
        Manager().signalSelectNewNavigationItem.connect(self.onSelectNewNavigationItem)
        
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


    def setupToolBars(self, standardToolBar, parent):
        pass


    def onNewNavigationItem(self, itemInfo):
        # itemInfo: id, name, type, (status), (refType), (refId), (schema)
        self.__twNavigation.createNewItem(itemInfo, True)


    def onSelectNewNavigationItem(self, devicePath):
        self.__twNavigation.selectItem(devicePath)


    # TODO: this is not working anymore due to change of Model-View-Controller
    #def onSchemaFromFileAvailable(self):
    #    #filename
    #    filename = QFileDialog.getOpenFileName(None, "Open Master Configuration", QDir.tempPath(), "XML-Schema (*.xs *.xsd)")
    #    file = QFile(filename)
    #    if file.open(QIODevice.ReadOnly | QIODevice.Text) == False:
    #        return

    #    schema = ""
    #    while file.atEnd() == False:
    #        schema.append(file.readLine())

    #    fi = QFileInfo(filename)
    #    rootKey = str(fi.baseName())
    #    model = self.__twNavigation.model()
    #    rootItem = model.invisibleRootItem()
    #    rootItem.fullKey = rootKey
    #    itemInfo = dict(id=-1, name=rootKey, key=rootKey, schema=schema, type=NavigationItemTypes.CLASS)
    #    Manager().onNewNavigationItem(itemInfo)


    def updateNavigationTreeView(self, config):
        self.__twNavigation.updateTreeModel(config)
        self.__twNavigation.expandAll()

        
    def onGlobalAccessLevelChanged(self):
        self.updateNavigationTreeView(Manager().treemodel.currentConfig)


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass

