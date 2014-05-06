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


from configuration import Configuration
from manager import Manager
from project import Project, Scene, Category
from projectmodel import ProjectModel

from PyQt4.QtCore import (pyqtSignal, QDir, QFile, QFileInfo, QIODevice, Qt)
from PyQt4.QtGui import (QAction, QCursor, QDialog, QFileDialog, QInputDialog,
                         QLineEdit, QMenu, QTreeView)


class ProjectTreeView(QTreeView):

    # To import a plugin a server connection needs to be established
    signalAddScene = pyqtSignal(str) # scene title
    signalItemChanged = pyqtSignal(object)
    signalSelectionChanged = pyqtSignal(list)


    def __init__(self, parent=None):
        super(ProjectTreeView, self).__init__(parent)

        # Set same mode for each project view
        self.setModel(Manager().projectTopology)
        self.expandAll()
        self.model().modelReset.connect(self.expandAll)
        self.setSelectionModel(self.model().selectionModel)
        self.model().selectionModel.selectionChanged.connect(self.onSelectionChanged)

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self.onCustomContextMenuRequested)


    def setupDefaultProject(self):
        """
        This function sets up a default project.
        So basically a new project is created and saved in the users temp folder.
        Previous data is overwritten.
        """
        projectName = "default_project"
        sceneName = "default_scene"
        directory = QDir.tempPath()

        project = self.model().createNewProject(projectName, directory)
        self.model().addScene(project, sceneName)
        project.save(True) # TODO: if default already exists: open existing

    
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


    def newProject(self):
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
            self.newProject()
            return

        projectName = projectName[0]

        directory = self.getProjectDir()
        if directory is None:
            return

        project = self.model().createNewProject(projectName, directory)
        project.save()


    def openProject(self):
        filename = QFileDialog.getOpenFileName(None, "Open saved project", \
                                               QDir.tempPath(), "XML (*.xml)")
        if len(filename) < 1:
            return
        
        self.model().openProject(filename)


    def saveCurrentProject(self):
        self.model().currentProject().save()


    def mouseDoubleClickEvent(self, event):
        index = self.selectionModel().currentIndex()
        if index is None: return
        if not index.isValid(): return

        object = index.data(ProjectModel.ITEM_OBJECT)
        if isinstance(object, Configuration):
            self.model().editDevice(object)
        elif isinstance(object, Scene):
            self.model().openScene(scene)


### slots ###
    def onCustomContextMenuRequested(self, pos):
        index = self.selectionModel().currentIndex()

        if not index.isValid():
            return

        object = index.data(ProjectModel.ITEM_OBJECT)
        
        menu = None
        if isinstance(object, Category) and (object.displayName == Project.DEVICES_LABEL):
            # Devices menu
            text = "Add device"
            acImportPlugin = QAction(text, self)
            acImportPlugin.setStatusTip(text)
            acImportPlugin.setToolTip(text)
            acImportPlugin.triggered.connect(self.model().onEditDevice)

            menu = QMenu()
            menu.addAction(acImportPlugin)
        elif isinstance(object, Category) and (object.displayName == Project.SCENES_LABEL):
            # Scenes menu
            text = "Add scene"
            acAddScene = QAction(text, self)
            acAddScene.setStatusTip(text)
            acAddScene.setToolTip(text)
            acAddScene.triggered.connect(self.model().onEditScene)

            menu = QMenu()
            menu.addAction(acAddScene)
        elif isinstance(object, Configuration) or isinstance(object, Scene):
            text = "Edit"
            acEdit = QAction(text, self)
            acEdit.setStatusTip(text)
            acEdit.setToolTip(text)
            
            if isinstance(object, Configuration):
                acEdit.triggered.connect(self.model().onEditDevice)
            elif isinstance(object, Scene):
                acEdit.triggered.connect(self.model().onEditScene)
             
            text = "Remove"
            acRemove = QAction(text, self)
            acRemove.setStatusTip(text)
            acRemove.setToolTip(text)
            acRemove.triggered.connect(self.model().onRemove)
            
            menu = QMenu()
            menu.addAction(acEdit)
            menu.addAction(acRemove)
        
        if menu is None: return
        
        menu.exec_(QCursor.pos())


    def onSelectionChanged(self, selected, deselected):
        selectedIndexes = selected.indexes()
        # Send signal to projectPanel to update toolbar actions
        self.signalSelectionChanged.emit(selectedIndexes)
        
        if len(selectedIndexes) < 1:
            return

        index = selectedIndexes[0]

        object = index.data(ProjectModel.ITEM_OBJECT)
        if object is None: return
        if not isinstance(object, Configuration):
            return

        deviceId = object.futureHash.get("deviceId")

        if not self.model().checkSystemTopology():
            return

        # Check whether deviceId is already online
        if self.model().systemTopology.has("device.{}".format(deviceId)):
            conf = Manager().getDevice(deviceId)
        else:
            conf = object
        
        print "signalItemChanged", conf.path, conf.type
        self.signalItemChanged.emit(conf)

