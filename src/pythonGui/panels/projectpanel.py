#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on FJuly 11, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the project panel on the
   middle left of the MainWindow which is un/dockable.
   
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

__all__ = ["ProjectPanel"]

import const

from manager import Manager

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class ProjectPanel(QWidget):
    ##########################################
    # Dockable widget class used in DivWidget
    # Requires following interface:
    # 
    #def setupActions(self):
    #    pass
    #def setupToolBar(self, toolBar):
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
        self.__twProject.setHeaderLabels(QStringList(""))
        self.__twProject.itemSelectionChanged.connect(self.projectItemSelectionChanged)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.__twProject)
        
        Manager().notifier.signalCreateNewProjectConfig.connect(self.onCreateNewProjectConfig)


    def setupActions(self):
        pass


    def setupToolBar(self, toolBar):
        pass


    def projectItemSelectionChanged(self):
        print "projectItemSelectionChanged"


    def onCreateNewProjectConfig(self, customItem, serverId, classId):
        #print serverId, classId
        
        nbItems = self.__twProject.topLevelItemCount()
        
        item = QTreeWidgetItem(self.__twProject)
        path = str("server." + serverId + ".classes." + classId + ".configuration.deviceId")
        item.setData(0, const.INTERNAL_KEY, path)
        item.setText(0, QString("%1-%2-<>").arg(nbItems).arg(classId))
        
        customItem.signalValueChanged.connect(self.onAdditionalInfoChanged)
        
        for i in self.__twProject.selectedItems():
            i.setSelected(False)
        
        item.setSelected(True)


    def onAdditionalInfoChanged(self, key, deviceId):
        # When deviceId of customItem was changed
        for item in self.__twProject.selectedItems():
            if item.data(0, const.INTERNAL_KEY).toPyObject() == key:
                oldText = item.text(0)
                splittedText = str(oldText).split("-<")
                item.setText(0, QString("%1-<%2>").arg(splittedText[0]).arg(deviceId))
                


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass


