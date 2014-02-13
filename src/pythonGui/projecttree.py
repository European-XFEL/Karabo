#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 4, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents the treewidget of the project and
configuration panel containing the parameters of a device.
"""

__all__ = ["ProjectTree"]


from enums import NavigationItemTypes
from manager import Manager
from karabo.karathon import Hash
from plugindialog import PluginDialog

from PyQt4.QtCore import (pyqtSignal, QDir, Qt)
from PyQt4.QtGui import (QAction, QCursor, QDialog, QFileDialog, QIcon,
                         QInputDialog, QLineEdit, QMenu, QMessageBox,
                         QTreeWidget, QTreeWidgetItem)


class ProjectTree(QTreeWidget):

    # To import a plugin a server connection needs to be established
    signalConnectToServer = pyqtSignal()

    ITEM_KEY = Qt.UserRole
    ITEM_SERVER_ID = Qt.UserRole + 1
    ITEM_CLASS_ID = Qt.UserRole + 2

    def __init__(self, parent):
        super(ProjectTree, self).__init__(parent)

        # Hash contains server/plugin topology
        self.__serverTopology = None

        # Dialog to add and change a device
        self.__pluginDialog = None

        self.setHeaderLabels(["Projects"])
        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self.onCustomContextMenuRequested)
        self.itemSelectionChanged.connect(self.onItemSelectionChanged)

        Manager().signalProjectHashChanged.connect(self.onUpdate)
        #Manager().signalSystemTopologyChanged.connect(self.onSystemTopologyChanged)


    def _createNewProject(self, projectName, directory):
        """
        This function creates a new project in the panel.
        """
        # Project name to lower case
        projectName = str(projectName).lower()

        projectConfig = Hash("project")
        projectConfig.setAttribute("project", "name", projectName)

        deviceLabel = "Devices"
        projectConfig.set("project.devices", Hash())
        projectConfig.setAttribute("project.devices", "label", deviceLabel)
        sceneLabel = "Scenes"
        projectConfig.set("project.scenes", Hash())
        projectConfig.setAttribute("project.scenes", "label", sceneLabel)
        macroLabel = "Macros"
        projectConfig.set("project.macros", Hash())
        projectConfig.setAttribute("project.macros", "label", macroLabel)
        monitorLabel = "Monitors"
        projectConfig.set("project.monitors", Hash())
        projectConfig.setAttribute("project.monitors", "label", monitorLabel)
        resourceLabel = "Resources"
        projectConfig.set("project.resources", Hash())
        projectConfig.setAttribute("project.resources", "label", resourceLabel)

        absoluteProjectPath = directory + "/" + projectName
        dir = QDir()
        if not QDir(absoluteProjectPath).exists():
            dir.mkpath(absoluteProjectPath)
        else:
            self._clearProjectDir(absoluteProjectPath)

        # Add subfolders
        dir.mkpath(absoluteProjectPath + "/" + deviceLabel)
        dir.mkpath(absoluteProjectPath + "/" + sceneLabel)
        dir.mkpath(absoluteProjectPath + "/" + macroLabel)
        dir.mkpath(absoluteProjectPath + "/" + monitorLabel)
        dir.mkpath(absoluteProjectPath + "/" + resourceLabel)

        # Send changes to manager
        Manager().addNewProject(projectName, directory, projectConfig)


    def _clearProjectDir(self, absolutePath):
        if len(absolutePath) < 1:
            return

        dirToDelete = QDir(absolutePath)
        # Remove all files from directory
        fileEntries = dirToDelete.entryList(QDir.Files | QDir.CaseSensitive)
        while len(fileEntries) > 0:
            dirToDelete.remove(fileEntries.pop())

        # Remove all sub directories
        dirEntries = dirToDelete.entryList(QDir.AllDirs | QDir.NoDotAndDotDot | QDir.CaseSensitive)
        while len(dirEntries) > 0:
            subDirPath = absolutePath + "/" + dirEntries.pop()
            subDirToDelete = QDir(subDirPath)
            if len(subDirToDelete.entryList()) > 0:
                self._clearProjectDir(subDirPath)
            subDirToDelete.rmpath(subDirPath)


    def _rFindItem(self, item, path):
        for i in range(item.childCount()):
            childItem = item.child(i)
            result = self._rFindItem(childItem, path)
            if (result is not None):
                return result

        itemPath = item.data(0, ProjectTree.ITEM_KEY)
        if itemPath == path:
            return item
        return None


    def _findItem(self, path):
        return self._rFindItem(self.invisibleRootItem(), path)


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
            self.onProjectNew()
            return

        directory = QFileDialog.getExistingDirectory(self, "Saving location of project", \
                        "/tmp/", QFileDialog.ShowDirsOnly | QFileDialog.DontResolveSymlinks)
        if not directory:
            return

        self._createNewProject(projectName[0], directory)


    def openProject(self):
        print "openProject"


    def saveProject(self):
        print "saveProject"


    def serverConnectionChanged(self, isConnected):
        print "serverConnectionChanged", isConnected
        if not isConnected:
            self.__serverTopology = None


### slots ###
    def onCustomContextMenuRequested(self, pos):
        item = self.itemAt(pos)
        if item is None:
            return

        if item.text(0) == "Devices":
            # Show devices menu
            menu = QMenu()
            text = "Add device"
            acImportPlugin = QAction(QIcon(":device-class"), text, None)
            acImportPlugin.setStatusTip(text)
            acImportPlugin.setToolTip(text)
            acImportPlugin.triggered.connect(self.onAddDevice)

            menu.addAction(acImportPlugin)
            menu.exec_(QCursor.pos())


    def onItemSelectionChanged(self):
        item = self.currentItem()
        if not item: return

        print ""
        print "++++ onItemSelectionChanged", item.text(0)

        path = item.data(0, ProjectTree.ITEM_KEY)
        if not path:
            return

        serverId = item.data(0, ProjectTree.ITEM_SERVER_ID)
        classId = item.data(0, ProjectTree.ITEM_CLASS_ID)
        # Get schema
        schema = Manager().getClassSchema(serverId, classId)
        print "schema is None?", schema is None
        print "serverId:", serverId, "classId:", classId
        print "path", path
        print ""
        # TODO: check whether deviceId is already online!!!
        itemInfo = dict(key=path, classId=classId, type=NavigationItemTypes.CLASS, schema=schema)
        Manager().onSchemaAvailable(itemInfo)
        Manager().onProjectItemChanged(itemInfo)


    def onAddDevice(self):
        if not self.__serverTopology or self.__serverTopology.empty():
            reply = QMessageBox.question(self, "No server connection",
                                         "Do you want to establish a server connection?",
                                         QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

            if reply == QMessageBox.No:
                return
            self.signalConnectToServer.emit()

        # Show dialog to select plugin
        self.__pluginDialog = PluginDialog(self.__serverTopology)
        if self.__pluginDialog.exec_() == QDialog.Rejected:
            return

        currentItem = self.currentItem()
        if currentItem:
            projectItem = currentItem.parent()
            if projectItem:
                # Path for device in project hash
                devicePath = projectItem.text(0) + ".project.devices." + \
                             self.__pluginDialog.deviceId
                # Path for device configuration
                configPath = str(devicePath + "." + self.__pluginDialog.plugin)

                # Put info in Hash
                config = Hash()
                config.set(configPath + ".deviceId", self.__pluginDialog.deviceId)
                config.set(configPath + ".serverId", self.__pluginDialog.server)
                # Add device to project hash
                Manager().addDeviceToProject(config)

                # Select added device
                item = self._findItem(devicePath)
                self.setCurrentItem(item)

        self.__pluginDialog = None


    def onUpdate(self, projectHash):
        print ""
        print "###################"
        print projectHash
        print ""
        self.blockSignals(True)
        self.clear()
        self.blockSignals(False)

        # Project hash structure:
        # projectName directory="directory" +
        #   project name="projectName" +
        #     devices label="Devices" +
        #     scenes label="Scenes" +
        #     macros label="Macros" +
        #     monitors label="Monitors" +
        #     resources label="Resources" +

        # Add child items
        for k in projectHash.keys():
            # Project names - toplevel items
            item = QTreeWidgetItem(self)
            item.setText(0, k)
            font = item.font(0)
            font.setBold(True)
            item.setFont(0, font)
            item.setIcon(0, QIcon(":folder"))
            item.setExpanded(True)

            projectConfig = projectHash.get(k)
            for l in projectConfig.keys():
                # Project key

                # Get children
                categoryConfig = projectConfig.get(l)
                for m in categoryConfig.keys():
                    # Categories - sub items
                    childItem = QTreeWidgetItem(item, [categoryConfig.getAttribute(m, "label")])
                    childItem.setIcon(0, QIcon(":folder"))
                    childItem.setExpanded(True)

                    subConfig = categoryConfig.get(m)
                    if subConfig.empty():
                        continue

                    for n in subConfig.keys():
                        leafItem = QTreeWidgetItem(childItem, [n])
                        # TODO: update icon on availability of device
                        leafItem.setIcon(0, QIcon(":device-instance"))

                        deviceId = k + "." + l + "." + m + "." + n
                        leafItem.setData(0, ProjectTree.ITEM_KEY, deviceId)

                        classConfig = subConfig.get(n)
                        for classId in classConfig.keys():
                            serverId = classConfig.get(classId + ".serverId")
                            # Set server and class ID
                            leafItem.setData(0, ProjectTree.ITEM_SERVER_ID, serverId)
                            leafItem.setData(0, ProjectTree.ITEM_CLASS_ID, classId)


    def onSystemTopologyChanged(self, config):
        serverKey = "server"
        if not config.has(serverKey):
            return

        self.__serverTopology = config.get(serverKey)
        if self.__pluginDialog:
            self.__pluginDialog.updateServerTopology(self.__serverTopology)

