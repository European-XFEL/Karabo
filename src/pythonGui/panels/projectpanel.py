#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 11, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the project panel on the
   middle left of the MainWindow which is un/dockable.
"""

__all__ = ["ProjectPanel"]

import const

from enums import NavigationItemTypes
from manager import Manager

#from PyQt4.QtCore import *
from PyQt4.QtGui import QTreeWidget, QTreeWidgetItem, QVBoxLayout, QWidget


class ProjectPanel(QWidget):
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
        super(ProjectPanel, self).__init__()
        
        title = "Projects"
        self.setWindowTitle(title)
        
        self.__twProject = QTreeWidget(self)
        self.__twProject.setHeaderLabels([])
        self.__twProject.itemSelectionChanged.connect(self.projectItemSelectionChanged)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.__twProject)
        
        Manager().signalCreateNewProjectConfig.connect(self.onCreateNewProjectConfig)


    def setupActions(self):
        pass


    def setupToolBars(self, standardToolBar, parent):
        pass


    def projectItemSelectionChanged(self):
        item = self.__twProject.currentItem()
        if not item: return
        Manager().signalProjectItemChanged.emit(dict(type=NavigationItemTypes.CLASS, key=item.data(0, const.INTERNAL_KEY)))


    def onCreateNewProjectConfig(self, customItem, path, configName):
        #print serverId, classId
        
        item = QTreeWidgetItem(self.__twProject)
        item.setData(0, const.INTERNAL_KEY, path)
        item.setText(0, configName)
        
        customItem.signalValueChanged.connect(self.onAdditionalInfoChanged)
        
        for i in self.__twProject.selectedItems():
            i.setSelected(False)
        
        item.setSelected(True)


    def onAdditionalInfoChanged(self, key, deviceId):
        # When deviceId of customItem was changed
        for i in xrange(self.__twProject.topLevelItemCount()):
            item = self.__twProject.topLevelItem(i)
            if (item.data(0, const.INTERNAL_KEY) + ".configuration.deviceId") == key:
                oldText = item.text(0)
                splittedText = str(oldText).split("-<")
                item.setText(0, "{}-<{}>").format(splittedText[0], deviceId)


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass


