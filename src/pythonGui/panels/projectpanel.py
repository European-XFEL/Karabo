#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 11, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the project panel on the
   middle left of the MainWindow which is un/dockable.
"""

__all__ = ["ProjectPanel"]

from projecttree import ProjectTree

from enums import NavigationItemTypes
from manager import Manager

from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import (QAction, QIcon, QTreeWidget, QTreeWidgetItem,
                         QVBoxLayout, QWidget)


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

    # To import a plugin a server connection needs to be established
    signalConnectToServer = pyqtSignal()

    def __init__(self):
        super(ProjectPanel, self).__init__()
        
        title = "Projects"
        self.setWindowTitle(title)

        self.__twProject = ProjectTree(self)
        self.__twProject.signalConnectToServer.connect(self.signalConnectToServer)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.__twProject)

        self.setupActions()


    def setupActions(self):
        text = "New project"
        self.__acProjectNew = QAction(QIcon(":new"), "&New project", self)
        self.__acProjectNew.setStatusTip(text)
        self.__acProjectNew.setToolTip(text)
        self.__acProjectNew.triggered.connect(self.onProjectNew)

        text = "Open project"
        self.__acProjectOpen = QAction(QIcon(":open"), "&Open project", self)
        self.__acProjectOpen.setStatusTip(text)
        self.__acProjectOpen.setToolTip(text)
        self.__acProjectOpen.triggered.connect(self.onProjectOpen)

        text = "Save project"
        self.__acProjectSave = QAction(QIcon(":save"), "&Save project", self)
        self.__acProjectSave.setStatusTip(text)
        self.__acProjectSave.setToolTip(text)
        self.__acProjectSave.setEnabled(False)
        self.__acProjectSave.triggered.connect(self.onProjectSave)


    def setupToolBars(self, standardToolBar, parent):
        standardToolBar.addAction(self.__acProjectNew)
        standardToolBar.addAction(self.__acProjectOpen)
        standardToolBar.addAction(self.__acProjectSave)


### slots ###
    def onProjectNew(self):
        self.__twProject.newProject()


    def onProjectOpen(self):
        self.__twProject.openProject()


    def onProjectSave(self):
        self.__twProject.saveProject()


    def onServerConnectionChanged(self, isConnected):
        self.__twProject.serverConnectionChanged(isConnected)


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass


