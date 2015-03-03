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
import icons
import manager

from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import (QAction, QVBoxLayout, QWidget)


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

    signalAddScene = pyqtSignal(object) # scene
    signalRemoveScene = pyqtSignal(object) # scene
    signalRenameScene = pyqtSignal(object) # scene
    signalAddMacro = pyqtSignal(object)
    signalRemoveMacro = pyqtSignal(object) # macro


    def __init__(self):
        super(ProjectPanel, self).__init__()
        
        title = "Projects"
        self.setWindowTitle(title)

        self.twProject = ProjectTreeView(self)
        self.twProject.model().signalAddScene.connect(self.signalAddScene)
        self.twProject.model().signalRemoveScene.connect(self.signalRemoveScene)
        self.twProject.model().signalRenameScene.connect(self.signalRenameScene)
        self.twProject.model().signalAddMacro.connect(self.signalAddMacro)
        self.twProject.model().signalRemoveMacro.connect(self.signalRemoveMacro)
        self.twProject.model().signalSelectionChanged.connect(self.onSelectionChanged)
        # Connect signal to get project available
        manager.Manager().signalAvailableProjects.connect(self.twProject.onAvailableProjects)
        manager.Manager().signalProjectLoaded.connect(self.twProject.onProjectLoaded)
        manager.Manager().signalProjectSaved.connect(self.twProject.onProjectSaved)
        manager.Manager().signalProjectClosed.connect(self.twProject.onProjectClosed)
        manager.Manager().signalReset.connect(self.onResetPanel)

        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.twProject)

        self.setupActions()


    def setupActions(self):
        text = "New project"
        self.acProjectNew = QAction(icons.new, "&New project", self)
        self.acProjectNew.setStatusTip(text)
        self.acProjectNew.setToolTip(text)
        self.acProjectNew.setEnabled(False)
        self.acProjectNew.triggered.connect(self.twProject.projectNew)

        text = "Open project"
        self.acProjectOpen = QAction(icons.open, "&Open project", self)
        self.acProjectOpen.setStatusTip(text)
        self.acProjectOpen.setToolTip(text)
        self.acProjectOpen.setEnabled(False)
        self.acProjectOpen.triggered.connect(self.twProject.projectOpen)

        text = "Save project"
        self.acProjectSave = QAction(icons.save, "&Save project", self)
        self.acProjectSave.setStatusTip(text)
        self.acProjectSave.setToolTip(text)
        self.acProjectSave.setEnabled(False)
        self.acProjectSave.triggered.connect(self.twProject.projectSave)

        text = "Save project as"
        self.acProjectSaveAs = QAction(icons.saveAs, "&Save project as", self)
        self.acProjectSaveAs.setStatusTip(text)
        self.acProjectSaveAs.setToolTip(text)
        self.acProjectSaveAs.setEnabled(False)
        self.acProjectSaveAs.triggered.connect(self.twProject.projectSaveAs)


    def setupToolBars(self, standardToolBar, parent):
        standardToolBar.addAction(self.acProjectNew)
        standardToolBar.addAction(self.acProjectOpen)
        standardToolBar.addAction(self.acProjectSave)
        standardToolBar.addAction(self.acProjectSaveAs)


    def closeAllProjects(self):
        return self.twProject.closeAllProjects()


    def enableToolBar(self, enabled):
        self.acProjectNew.setEnabled(enabled)
        self.acProjectOpen.setEnabled(enabled)


    def onSelectionChanged(self, selectedIndexes):
        self.acProjectSave.setEnabled(len(selectedIndexes) > 0)
        self.acProjectSaveAs.setEnabled(len(selectedIndexes) > 0)


    def onResetPanel(self):
        self.closeAllProjects()


    def onUndock(self):
        pass


    def onDock(self):
        pass


