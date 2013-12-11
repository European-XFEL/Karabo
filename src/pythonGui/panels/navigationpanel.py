#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the navigation panel on the
   left of the MainWindow which is un/dockable.
   
   As a dockable widget class used in DivWidget, it needs the following interfaces
   implemented:
   
    def setupActions(self):
        pass
    def setupToolBar(self, toolBar):
        pass
    def onUndock(self):
        pass
    def onDock(self):
        pass
"""

__all__ = ["NavigationPanel"]


import const

from enums import NavigationItemTypes
from manager import Manager
from navigationtreeview import NavigationTreeView

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class NavigationPanel(QWidget):
    # signals
    signalNavigationItemChanged = pyqtSignal(dict) # type, key


    def __init__(self, treemodel):
        super(NavigationPanel, self).__init__()
        
        title = "Navigation"
        self.setWindowTitle(title)
                
        self.__twNavigation = NavigationTreeView(self, treemodel)
        self.connect(self.__twNavigation.selectionModel(),
                    SIGNAL('selectionChanged(const QItemSelection &, const QItemSelection &)'),
                    self.onNavigationItemClicked)
        
        # Make connects
        Manager().notifier.signalSystemTopologyChanged.connect(self.onSystemTopologyChanged)
        
        Manager().notifier.signalGlobalAccessLevelChanged.connect(self.onGlobalAccessLevelChanged)
        
        Manager().notifier.signalNewNavigationItem.connect(self.onNewNavigationItem)
        Manager().notifier.signalSelectNewNavigationItem.connect(self.onSelectNewNavigationItem)
        Manager().notifier.signalNavigationItemChanged.connect(self.onNavigationItemChanged)
        Manager().notifier.signalNavigationItemSelectionChanged.connect(self.onNavigationItemSelectionChanged)
        
        Manager().notifier.signalInstanceGone.connect(self.onInstanceGone)
        
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
        #text = "Load master configuration (*.xsd)"
        #self.__acLoadExpectedParameters = QAction(QIcon(":configure"), "&Open XSD", self)
        #self.__acLoadExpectedParameters.setToolTip(text)
        #self.__acLoadExpectedParameters.setStatusTip(text)
        #self.__acLoadExpectedParameters.setShortcuts(QKeySequence.New)
        #self.__acLoadExpectedParameters.triggered.connect(self.onSchemaFromFileAvailable)


    def setupToolBar(self, toolBar):
        pass
        #toolBar.addAction(self.__acLoadExpectedParameters)


### slots ###
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

    #    schema = QString()
    #    while file.atEnd() == False:
    #        schema.append(file.readLine())

    #    fi = QFileInfo(filename)
    #    rootKey = str(fi.baseName())
    #    model = self.__twNavigation.model()
    #    rootItem = model.invisibleRootItem()
    #    rootItem.fullKey = rootKey
    #    itemInfo = dict(id=-1, name=rootKey, key=rootKey, schema=schema, type=NavigationItemTypes.CLASS)
    #    Manager().onNewNavigationItem(itemInfo)


    # NavigationTreeView: selectionChanged(const QItemSelection &, const QItemSelection &)
    def onNavigationItemClicked(self):
        #print "NavigationPanel.itemClicked"
        self.__twNavigation.itemClicked()


    # signal from Manager NavigationTreeWidgetItem clicked (ConfigurationPanel)
    def onNavigationItemChanged(self, itemInfo):
        #print "NavigationPanel.itemChanged"
        self.__twNavigation.itemChanged(itemInfo)


    def onNavigationItemSelectionChanged(self, path):
        self.__twNavigation.selectItem(path)


    def onInstanceGone(self, path, parentPath):
        self.__twNavigation.selectItem(str(parentPath))
        
        
    def onSystemTopologyChanged(self, config):
        self.__twNavigation.updateView(config)


    def onGlobalAccessLevelChanged(self):
        self.__twNavigation.updateView(Manager().treemodel.currentConfig)


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass

