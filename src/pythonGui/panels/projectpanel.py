#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 11, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the project panel on the
   middle left of the MainWindow which is un/dockable.
"""

__all__ = ["ProjectPanel"]

from projecttreeview import ProjectTreeView

from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import (QAction, QIcon, QVBoxLayout, QWidget)


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
    signalServerConnection = pyqtSignal(bool)
    signalAddScene = pyqtSignal(object) # scene
    signalOpenScene = pyqtSignal(object) # scene
    signalSaveScene = pyqtSignal(object) # scene

    def __init__(self):
        super(ProjectPanel, self).__init__()
        
        title = "Projects"
        self.setWindowTitle(title)

        self.twProject = ProjectTreeView(self)
        self.twProject.model().signalAddScene.connect(self.signalAddScene)
        self.twProject.model().signalOpenScene.connect(self.signalOpenScene)
        self.twProject.model().signalSaveScene.connect(self.signalSaveScene)
        self.twProject.model().signalServerConnection.connect(self.signalServerConnection)
        self.twProject.model().signalSelectionChanged.connect(self.onSelectionChanged)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.twProject)

        self.setupActions()


    def setupActions(self):
        text = "New project"
        self.acProjectNew = QAction(QIcon(":new"), "&New project", self)
        self.acProjectNew.setStatusTip(text)
        self.acProjectNew.setToolTip(text)
        self.acProjectNew.triggered.connect(self.onProjectNew)

        text = "Open project"
        self.acProjectOpen = QAction(QIcon(":open"), "&Open project", self)
        self.acProjectOpen.setStatusTip(text)
        self.acProjectOpen.setToolTip(text)
        self.acProjectOpen.triggered.connect(self.onProjectOpen)

        text = "Save project"
        self.acProjectSave = QAction(QIcon(":save"), "&Save project", self)
        self.acProjectSave.setStatusTip(text)
        self.acProjectSave.setToolTip(text)
        self.acProjectSave.setEnabled(False)
        self.acProjectSave.triggered.connect(self.onProjectSave)

        text = "Save project as"
        self.acProjectSaveAs = QAction(QIcon(":save-as"), "&Save project as", self)
        self.acProjectSaveAs.setStatusTip(text)
        self.acProjectSaveAs.setToolTip(text)
        self.acProjectSaveAs.setEnabled(False)
        self.acProjectSaveAs.triggered.connect(self.onProjectSaveAs)


    def setupToolBars(self, standardToolBar, parent):
        standardToolBar.addAction(self.acProjectNew)
        standardToolBar.addAction(self.acProjectOpen)
        standardToolBar.addAction(self.acProjectSave)
        standardToolBar.addAction(self.acProjectSaveAs)


    def setupDefaultProject(self):
        self.twProject.setupDefaultProject()


### slots ###
    def onProjectNew(self):
        self.twProject.projectNew()


    def onProjectOpen(self):
        self.twProject.projectOpen()


    def onProjectSave(self):
        self.twProject.projectSave()


    def onProjectSaveAs(self):
        self.twProject.projectSaveAs()


    def onSelectionChanged(self, selectedIndexes):
        self.acProjectSave.setEnabled(len(selectedIndexes) > 0)
        self.acProjectSaveAs.setEnabled(len(selectedIndexes) > 0)


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass


