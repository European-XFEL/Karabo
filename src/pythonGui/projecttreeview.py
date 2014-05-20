from __future__ import unicode_literals
#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 4, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the treewidget of the project and
configuration panel containing the parameters of a device.
"""

__all__ = ["ProjectTreeView"]


from manager import Manager
from project import Category, Device, Project, Scene
from projectmodel import ProjectModel

from PyQt4.QtCore import (pyqtSignal, QDir, QFile, QFileInfo, QIODevice, Qt)
from PyQt4.QtGui import (QAction, QCursor, QDialog, QFileDialog, QInputDialog,
                         QLineEdit, QMenu, QTreeView)
import os.path


class ProjectTreeView(QTreeView):


    def __init__(self, parent=None):
        super(ProjectTreeView, self).__init__(parent)

        # Set same mode for each project view
        self.setModel(Manager().projectTopology)
        self.expandAll()
        self.model().modelReset.connect(self.expandAll)
        self.setSelectionModel(self.model().selectionModel)

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self.onCustomContextMenuRequested)


    def currentIndex(self):
        return self.selectionModel().currentIndex()


    def indexInfo(self, index=None):
        """
        This function return the info about the given \index.

        Defaults to the current index, if index is None.
        """
        if index is None:
            index = self.currentIndex()
        return self.model().indexInfo(index)


    def setupDefaultProject(self):
        """
        This function sets up the default project.
        
        If the default project already exists in the given directory it is opened,
        otherwise a new default project is created.
        """
        projectName = "default_project"
        directory = QDir.tempPath()
        
        projectFile = "{}.KRB".format(projectName)
        alreadyExists = self.model().projectExists(directory, projectFile)
        if alreadyExists:
            # Open existing default project
            filename = os.path.join(directory, projectFile)
            self.model().projectOpen(filename)
            return

        # Create new project or overwrite existing
        project = self.model().createNewProject(projectName, directory)
        self.model().addScene(project, "default_scene")
        project.zip()


    def closeAllProjects(self):
        self.model().closeAllProjects()

    
    def getProjectDir(self):
        fileDialog = QFileDialog(self, "Save project", QDir.tempPath())
        fileDialog.setFileMode(QFileDialog.Directory)
        fileDialog.setOptions(QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)

        if fileDialog.exec_() == QDialog.Rejected:
            return None
        
        directory = fileDialog.selectedFiles()
        if len(directory) < 0:
            return None
        
        return directory[0]


    def projectNew(self):
        projectName = QInputDialog.getText(self, "New project", \
                                           "Enter project name:", QLineEdit.Normal, "")

        if not projectName[1]:
            return

        if len(projectName[0]) < 1:
            reply = QMessageBox.question(self, "Project name", "Please enter a name!",
                QMessageBox.Ok | QMessageBox.Cancel, QMessageBox.Ok)

            if reply == QMessageBox.Cancel:
                return

            # Call function again
            self.projectNew()
            return

        projectName = projectName[0]

        directory = self.getProjectDir()
        if directory is None:
            return

        project = self.model().createNewProject(projectName, directory)
        project.zip()


    def projectOpen(self):
        filename = QFileDialog.getOpenFileName(None, "Open project", \
                                               QDir.tempPath(), "KRB (*.KRB)")
        if len(filename) < 1:
            return
        
        self.model().projectOpen(filename)


    def projectSave(self):
        self.model().projectSave()


    def projectSaveAs(self):
        directory = self.getProjectDir()
        self.model().projectSaveAs(directory)


    def mouseDoubleClickEvent(self, event):
        index = self.currentIndex()
        if not index.isValid(): return

        object = index.data(ProjectModel.ITEM_OBJECT)
        if isinstance(object, Device):
            self.model().editDevice(object)
        elif isinstance(object, Scene):
            self.model().showScene(object)


### slots ###
    def onCustomContextMenuRequested(self, pos):
        index = self.currentIndex()
        if not index.isValid(): return

        object = index.data(ProjectModel.ITEM_OBJECT)
        
        menu = None
        if isinstance(object, Category) and (object.displayName == Project.DEVICES_LABEL):
            # Devices menu
            text = "Add device"
            acImportPlugin = QAction(text, self)
            acImportPlugin.setStatusTip(text)
            acImportPlugin.setToolTip(text)
            acImportPlugin.triggered.connect(self.model().onEditDevice)

            text = "Initiate all"
            acInitDevices = QAction(text, self)
            acInitDevices.setStatusTip(text)
            acInitDevices.setToolTip(text)
            acInitDevices.triggered.connect(self.model().onInitDevices)

            text = "Kill all"
            acKillDevices = QAction(text, self)
            acKillDevices.setStatusTip(text)
            acKillDevices.setToolTip(text)
            acKillDevices.triggered.connect(self.model().onKillDevices)

            menu = QMenu()
            menu.addAction(acImportPlugin)
            menu.addSeparator()
            menu.addAction(acInitDevices)
            menu.addAction(acKillDevices)
        elif isinstance(object, Category) and (object.displayName == Project.SCENES_LABEL):
            # Scenes menu
            text = "Add scene"
            acAddScene = QAction(text, self)
            acAddScene.setStatusTip(text)
            acAddScene.setToolTip(text)
            acAddScene.triggered.connect(self.model().onEditScene)

            menu = QMenu()
            menu.addAction(acAddScene)
        elif isinstance(object, Device) or isinstance(object, Scene):
            text = "Edit"
            acEdit = QAction(text, self)
            acEdit.setStatusTip(text)
            acEdit.setToolTip(text)
            
            if isinstance(object, Device):
                acEdit.triggered.connect(self.model().onEditDevice)
            elif isinstance(object, Scene):
                acEdit.triggered.connect(self.model().onEditScene)
             
            text = "Remove"
            acRemove = QAction(text, self)
            acRemove.setStatusTip(text)
            acRemove.setToolTip(text)
            acRemove.triggered.connect(self.model().onRemove)
            
            text = "Instantiate"
            acInitDevice = QAction(text, self)
            acInitDevice.setStatusTip(text)
            acInitDevice.setToolTip(text)
            acInitDevice.triggered.connect(self.onInitDevice)
            
            text = "Kill"
            acKillDevice = QAction(text, self)
            acKillDevice.setStatusTip(text)
            acKillDevice.setToolTip(text)
            acKillDevice.triggered.connect(self.onKillDevice)
            
            menu = QMenu()
            menu.addAction(acEdit)
            menu.addAction(acRemove)
            menu.addSeparator()
            menu.addAction(acInitDevice)
            menu.addAction(acKillDevice)
        
        if menu is None: return
        
        menu.exec_(QCursor.pos())


    def onInitDevice(self):
        index = self.currentIndex()
        object = index.data(ProjectModel.ITEM_OBJECT)
        if not isinstance(object, Device): return
        
        config = object.toHash()
        serverId = config.get("serverId")
        Manager().initDevice(serverId, object.classId, config)


    def onKillDevice(self):
        index = self.currentIndex()
        object = index.data(ProjectModel.ITEM_OBJECT)
        if not isinstance(object, Device): return
        
        Manager().killDevice(object.key)

