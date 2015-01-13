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

import globals

from dialogs.projectdialog import ProjectSaveDialog, ProjectLoadDialog
from scene import Scene
from manager import Manager
from network import Network
from guiproject import Category, Device, DeviceGroup, GuiProject, Macro
from projectmodel import ProjectModel
from util import getSaveFileName

from karabo.project import Project, ProjectConfiguration

from PyQt4.QtCore import Qt
from PyQt4.QtGui import (QAbstractItemView, QAction, QCursor, QDialog,
                         QFileDialog, QMenu, QMessageBox, QTreeView)
import os.path


class ProjectTreeView(QTreeView):


    def __init__(self, parent=None):
        super(ProjectTreeView, self).__init__(parent)

        # Set same mode for each project view
        self.setModel(Manager().projectTopology)
        self.model().signalExpandIndex.connect(self.setExpanded)
        self.setSelectionModel(self.model().selectionModel)
        self.setSelectionMode(QAbstractItemView.ExtendedSelection)
        #self.setDragDropMode(QAbstractItemView.DragDrop)#InternalMove)

        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self.onCustomContextMenuRequested)
        
        self.projectDialog = None


    def currentIndex(self):
        return self.model().currentIndex()


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
        filename = os.path.join(globals.KARABO_PROJECT_FOLDER, "default_project.krb")
        if os.path.exists(filename):
            project = self.model().projectOpen(filename)
            # default project should always have a default_scene
            if not project.scenes:
                self.model().addScene(project, "default_scene")
                project.zip()
        else:
            project = self.model().projectNew(filename)
            self.model().addScene(project, "default_scene")
            project.zip()


    def closeAllProjects(self):
        return self.model().closeAllProjects()


    def projectNew(self):
        self.projectDialog = ProjectSaveDialog()
        
        if self.projectDialog.exec_() == QDialog.Rejected:
            self.projectDialog = None
            return
        
        # TODO: Create project krb file and ship content
        Network().onSaveProject(self.projectDialog.filename)#, content)

        self.projectDialog = None
        return
    
        fn = getSaveFileName("New Project", globals.KARABO_PROJECT_FOLDER,
                             "Karabo Projects (*.krb)", "krb")

        if fn:
            self.model().projectNew(fn)


    def projectOpen(self):
        self.projectDialog = ProjectLoadDialog()
        
        if self.projectDialog.exec_() == QDialog.Rejected:
            self.projectDialog = None
            return
        
        Network().onLoadProject(self.projectDialog.filename)
        return
    
        filename = QFileDialog.getOpenFileName(
            None, "Open project", globals.KARABO_PROJECT_FOLDER,
            "Karabo Projects (*.krb)")

        if len(filename) < 1:
            return
        self.model().projectOpen(filename)


    def projectSave(self):
        self.model().projectSave()


    def projectSaveAs(self):
        fn = getSaveFileName("Save Project As", globals.KARABO_PROJECT_FOLDER,
                             "Karabo Projects (*.krb)", "krb")
        if fn:
            self.model().projectSaveAs(fn)


    def mouseDoubleClickEvent(self, event):
        index = self.currentIndex()
        if not index.isValid(): return

        object = index.data(ProjectModel.ITEM_OBJECT)
        if isinstance(object, Device) or isinstance(object, DeviceGroup):
            self.model().editDevice(object)
        elif isinstance(object, Scene):
            self.model().openScene(object)


### slots ###
    def onCustomContextMenuRequested(self, pos):
        selectedIndexes = self.selectionModel().selectedIndexes()
        if not selectedIndexes:
            return
        
        nbSelected = len(selectedIndexes)
        
        firstIndex = selectedIndexes[0]
        firstObj = firstIndex.data(ProjectModel.ITEM_OBJECT)
        # Make sure that selected indexes are of same type
        selectedType = type(firstObj)
        for index in selectedIndexes:
            object = index.data(ProjectModel.ITEM_OBJECT)
            if not isinstance(object, selectedType):
                return
        
        menu = QMenu()
        if selectedType is GuiProject:
            # Project menu
            if nbSelected > 1:
                text = "Close selected projects"
            else:
                text = "Close project"
            acCloseProject = QAction(text, self)
            acCloseProject.setStatusTip(text)
            acCloseProject.setToolTip(text)
            acCloseProject.triggered.connect(self.model().onCloseProject)

            menu.addAction(acCloseProject)
        elif selectedType is Category:
            if nbSelected > 1:
                return
            
            if firstObj.displayName == Project.DEVICES_LABEL:
                # Devices menu
                text = "Add device"
                acImportPlugin = QAction(text, self)
                acImportPlugin.setStatusTip(text)
                acImportPlugin.setToolTip(text)
                acImportPlugin.triggered.connect(self.model().onEditDevice)

                text = "Instantiate all"
                acInitDevices = QAction(text, self)
                acInitDevices.setStatusTip(text)
                acInitDevices.setToolTip(text)
                acInitDevices.triggered.connect(self.model().onInitDevices)

                text = "Shutdown all"
                acKillDevices = QAction(text, self)
                acKillDevices.setStatusTip(text)
                acKillDevices.setToolTip(text)
                acKillDevices.triggered.connect(self.model().onKillDevices)

                text = "Remove all"
                acRemoveDevices = QAction(text, self)
                acRemoveDevices.setStatusTip(text)
                acRemoveDevices.setToolTip(text)
                acRemoveDevices.triggered.connect(self.model().onRemoveDevices)
                
                menu.addAction(acImportPlugin)
                menu.addSeparator()
                menu.addAction(acInitDevices)
                menu.addAction(acKillDevices)
                menu.addSeparator()
                menu.addAction(acRemoveDevices)
            elif firstObj.displayName == Project.SCENES_LABEL:
                # Scenes menu
                text = "Add scene"
                acAddScene = QAction(text, self)
                acAddScene.setStatusTip(text)
                acAddScene.setToolTip(text)
                acAddScene.triggered.connect(self.model().onEditScene)

                text = "Open scene"
                acOpenScene = QAction(text, self)
                acOpenScene.setStatusTip(text)
                acOpenScene.setToolTip(text)
                acOpenScene.triggered.connect(self.model().onOpenScene)

                menu.addAction(acAddScene)
                menu.addAction(acOpenScene)
            elif firstObj.displayName == Project.MACROS_LABEL:
                text = "Add Macro"
                acAddMacro = QAction(text, self)
                acAddMacro.setStatusTip(text)
                acAddMacro.setToolTip(text)
                acAddMacro.triggered.connect(self.model().onEditMacro)


                text = "Load Macro"
                acLoadMacro = QAction(text, self)
                acLoadMacro.setStatusTip(text)
                acLoadMacro.setToolTip(text)
                acLoadMacro.triggered.connect(self.model().onLoadMacro)

                menu.addAction(acAddMacro)
                menu.addAction(acLoadMacro)
        elif selectedType in (Device, DeviceGroup, Scene, Macro):
            # Device or Scene menu
            if nbSelected > 1:
                text = "Remove selected"
            else:
                text = "Edit"
                acEdit = QAction(text, self)
                acEdit.setStatusTip(text)
                acEdit.setToolTip(text)

                text = "Duplicate"
                acDuplicate = QAction(text, self)
                acDuplicate.setStatusTip(text)
                acDuplicate.setToolTip(text)
                
                menu.addAction(acEdit)
                menu.addAction(acDuplicate)
                
                text = "Remove"
            
            acRemove = QAction(text, self)
            acRemove.setStatusTip(text)
            acRemove.setToolTip(text)
            acRemove.triggered.connect(self.model().onRemove)
            
            menu.addAction(acRemove)
            
            if selectedType in (Device, DeviceGroup):
                if nbSelected > 1:
                    text = "Instantiate selected"
                else:
                    acEdit.triggered.connect(self.model().onEditDevice)
                    acDuplicate.triggered.connect(self.model().onDuplicateDevice)
                
                    text = "Instantiate"
                
                acInitDevice = QAction(text, self)
                acInitDevice.setStatusTip(text)
                acInitDevice.setToolTip(text)
                acInitDevice.triggered.connect(self.onInitDevice)

                if nbSelected > 1:
                    text = "Shutdown selected"
                else:
                    text = "Shutdown"
                acKillDevice = QAction(text, self)
                acKillDevice.setStatusTip(text)
                acKillDevice.setToolTip(text)
                acKillDevice.triggered.connect(self.onKillDevice)
                
                menu.addSeparator()
                menu.addAction(acInitDevice)
                menu.addAction(acKillDevice)
            elif selectedType is Macro:
                acEdit.triggered.connect(self.model().onEditMacro)
                acDuplicate.triggered.connect(self.model().onDuplicateMacro)
            elif selectedType is Scene:
                if nbSelected == 1:
                    acEdit.triggered.connect(self.model().onEditScene)
                    acDuplicate.triggered.connect(self.model().onDuplicateScene)
                    text = "Save as..."
                    acSaveAs = QAction(text, self)
                    acSaveAs.setStatusTip(text)
                    acSaveAs.setToolTip(text)
                    acSaveAs.triggered.connect(self.model().onSaveAsScene)
                
                    menu.addSeparator()
                    menu.addAction(acSaveAs)
        
        elif selectedType is ProjectConfiguration:
            if nbSelected > 1:
                text = "Remove selected"
            else:
                # TODO
                #text = "Edit"
                #acEdit = QAction(text, self)
                #acEdit.setStatusTip(text)
                #acEdit.setToolTip(text)
                #acEdit.triggered.connect(self.model().onEditConfiguration)
                #menu.addAction(acEdit)
                text = "Remove"
            acRemove = QAction(text, self)
            acRemove.setStatusTip(text)
            acRemove.setToolTip(text)
            acRemove.triggered.connect(self.model().onRemoveConfiguration)
            menu.addAction(acRemove)

        menu.exec_(QCursor.pos())


    def onInitDevice(self):
        selectedIndexes = self.selectionModel().selectedIndexes()
        for index in selectedIndexes:
            device = index.data(ProjectModel.ITEM_OBJECT)
            device.project.instantiate(device)


    def onKillDevice(self):
        selectedIndexes = self.selectionModel().selectedIndexes()
        nbSelected = len(selectedIndexes)
        if nbSelected > 1:
            reply = QMessageBox.question(self, 'Shutdown selected devices',
                "Do you really want to shutdown all selected devices?",
                QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

            if reply == QMessageBox.No:
                return
        
        for index in selectedIndexes:
            device = index.data(ProjectModel.ITEM_OBJECT)
            device.project.shutdown(device, nbSelected == 1)


    def onAvailableProjects(self, projects):
        if self.projectDialog is None:
            return
        
        self.projectDialog.fillCloudProjects(projects)

