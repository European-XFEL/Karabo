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

from dialogs.projectdialog import ProjectDialog, ProjectSaveDialog, ProjectLoadDialog
from scene import Scene
from manager import Manager
from messagebox import MessageBox
from network import Network
from guiproject import Category, Device, DeviceGroup, GuiProject, Macro
from projectmodel import ProjectModel
from util import getSaveFileName

from karabo.project import Project, ProjectAccess, ProjectConfiguration

from PyQt4.QtCore import Qt
from PyQt4.QtGui import (QAbstractItemView, QAction, QCursor, QDialog,
                         QMenu, QMessageBox, QTreeView)
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


    def getProjectSaveName(self):
        """
        Returns a tuple containing the filepath, project name and the location
        (e.g. CLOUD, LOCAL).
        """
        self.projectDialog = ProjectSaveDialog()
        if self.projectDialog.exec_() == QDialog.Rejected:
            self.projectDialog = None
            return (None, None, None)
        
        projectName = self.projectDialog.filename
        filePath = globals.KARABO_PROJECT_FOLDER
        location = self.projectDialog.location
        
        return filePath, projectName, location


    def projectDataBytes(self, fileName):
        """
        This function returns the bytes of the file with the given \fileName,
        else None is returned.
        """
        # Read new project bytes
        data = None
        with open(fileName, 'rb') as input:
            data = input.read()
        
        input.close()
        self.projectDialog = None
        
        return data


    def projectNew(self):
        filePath, projectName, location = self.getProjectSaveName()
        if (filePath is None) and (projectName is None) and (location is None):
            return
        
        fileName = os.path.join(filePath, projectName)
        
        # Save project to local file system
        self.model().projectNew(fileName, \
                        ProjectAccess.LOCAL if location == ProjectDialog.LOCAL \
                                            else ProjectAccess.CLOUD)
        
        if location == ProjectDialog.CLOUD:
            data = self.projectDataBytes(fileName)
            # Send save project to cloud request to network
            Network().onNewProject(projectName, data)


    def projectOpen(self):
        self.projectDialog = ProjectLoadDialog()
        
        if self.projectDialog.exec_() == QDialog.Rejected:
            self.projectDialog = None
            return
        
        if self.projectDialog.location == ProjectDialog.CLOUD:
            Network().onLoadProject(self.projectDialog.filename)
        elif self.projectDialog.location == ProjectDialog.LOCAL:
            self.model().projectOpen(self.projectDialog.filepath, \
                                     ProjectAccess.LOCAL)


    def projectSave(self):
        project = self.model().currentProject()
        if project.access == ProjectAccess.CLOUD:
            msgBox = QMessageBox(QMessageBox.Question, "Check in project", 
                       "The project \"<b>{}</b>\" has been checked out.<br>"
                       "Do you want to check it in again or save it locally?".format(project.name))

            btnCheckIn = msgBox.addButton("Check in", QMessageBox.YesRole)
            btnLocally = msgBox.addButton("Save locally", QMessageBox.NoRole)
            btnAbort = msgBox.addButton("Cancel", QMessageBox.RejectRole)
            
            msgBox.exec_()
            resultBtn = msgBox.clickedButton()
            if resultBtn == btnCheckIn:
                self.model().projectSave()
                data = self.projectDataBytes(project.filename)
                # Send save project to cloud request to network
                Network().onSaveProject(project.basename, data)
            if resultBtn == btnLocally:
                self.model().projectSave()
            elif resultBtn == btnAbort:
                return
        elif project.access == ProjectAccess.CLOUD_READONLY:
            print("Project was saved locally.")
        


    def projectSaveAs(self):
        filePath, projectName, location = self.getProjectSaveName()
        if (filePath is None) and (projectName is None) and (location is None):
            return

        fileName = os.path.join(filePath, projectName)
        self.model().projectSaveAs(fileName)
        
        if location == ProjectDialog.CLOUD:
            data = self.projectDataBytes(fileName)
            # Send save project to cloud request to network
            Network().onNewProject(projectName, data)
            # TODO: close old project


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


    def onProjectLoaded(self, name, metaData, data):
        # Write cloud project to local file system
        filename = os.path.join(globals.KARABO_PROJECT_FOLDER, name)
        with open(filename, "wb") as out:
            out.write(data)
        out.close()
        
        checkedOut = metaData.get("checkedOut")
        self.model().projectOpen(filename, ProjectAccess.CLOUD_READONLY \
                                         if checkedOut else ProjectAccess.CLOUD)


    def onProjectSaved(self, name, success):
        if success:
            text = "Project <b>{}</b> saved successfully.".format(name)
        else:
            text = "Project <b>{}</b> could not be saved properly.".format(name)
        MessageBox.showInformation(text, "Project saved")


    def onProjectClosed(self, name, success):
        if success:
            text = "Project <b>{}</b> closed successfully.".format(name)
        else:
            text = "Project <b>{}</b> could not be closed properly.".format(name)
        MessageBox.showInformation(text, "Project closed")

