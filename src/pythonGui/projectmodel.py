#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 20, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""
This module contains a class which represents a model to display projects in
a treeview.
"""

__all__ = ["ProjectModel"]

from karabo.karathon import VectorHash

from PyQt4.QtCore import Qt
from PyQt4.QtGui import (QIcon, QItemSelectionModel, QStandardItem,
                         QStandardItemModel)


class ProjectModel(QStandardItemModel):

    ITEM_PATH = Qt.UserRole
    ITEM_CATEGORY = Qt.UserRole + 1
    ITEM_SERVER_ID = Qt.UserRole + 2
    ITEM_CLASS_ID = Qt.UserRole + 3

    PROJECT_KEY = "project"

    DEVICE_KEY = "device"
    SCENES_KEY = "scenes"
    MACROS_KEY = "macros"
    MONITORS_KEY = "monitors"
    RESOURCES_KEY = "resources"

    DEVICES_LABEL = "Devices"
    SCENES_LABEL = "Scenes"
    MACROS_LABEL = "Macros"
    MONITORS_LABEL = "Monitors"
    RESOURCES_LABEL = "Resources"


    def __init__(self, parent=None):
        super(ProjectModel, self).__init__(parent)

        self.setHorizontalHeaderLabels(["Projects"])
        self.selectionModel = QItemSelectionModel(self)


    def _handleLeafItems(self, childItem, projectPath, categoryKey, config, centralHash):
        if (config is None) or config.empty():
            return

        if categoryKey == ProjectModel.SCENES_KEY:
            fileName = config.get("filename")
            alias = config.get("alias")

            leafItem = QStandardItem(alias)
            leafItem.setEditable(False)
            leafPath = projectPath + "." + categoryKey + "." + alias
            leafItem.setData(leafPath, ProjectModel.ITEM_PATH)
            childItem.appendRow(leafItem)
        else:
            for leafKey in config.keys():
                leafItem = QStandardItem(leafKey)
                leafItem.setEditable(False)
                leafPath = projectPath + "." + categoryKey + "." + leafKey
                leafItem.setData(leafPath, ProjectModel.ITEM_PATH)
                childItem.appendRow(leafItem)

                if categoryKey == ProjectModel.DEVICE_KEY:
                    # TODO: Look for other possibilities than centralHash
                    # to check whether deviceId is on/offline
                    # Update icon on availability of device
                    if centralHash.has(ProjectModel.DEVICE_KEY + "." + leafKey):
                        leafItem.setIcon(QIcon(":device-instance"))
                    else:
                        leafItem.setIcon(QIcon(":offline"))

                    classConfig = config.get(leafKey)
                    for classId in classConfig.keys():
                        serverId = classConfig.get(classId + ".serverId")
                        # Set server and class ID
                        leafItem.setData(serverId, ProjectModel.ITEM_SERVER_ID)
                        leafItem.setData(classId, ProjectModel.ITEM_CLASS_ID)


    def updateData(self, projectHash, centralHash):
        print ""
        print projectHash
        print ""

        self.beginResetModel()
        self.clear()
        self.setHorizontalHeaderLabels(["Projects"])

        rootItem = self.invisibleRootItem()

        # Add child items
        for projectKey in projectHash.keys():
            # Project names - toplevel items
            item = QStandardItem(projectKey)
            item.setEditable(False)
            font = item.font()
            font.setBold(True)
            item.setFont(font)
            item.setIcon(QIcon(":folder"))
            rootItem.appendRow(item)

            projectConfig = projectHash.get(projectKey)
            for p in projectConfig.keys():
                # Project key

                # Get children
                categoryConfig = projectConfig.get(p)
                for categoryKey in categoryConfig.keys():
                    # Categories - sub items
                    childItem = QStandardItem(categoryConfig.getAttribute(categoryKey, "label"))
                    childItem.setEditable(False)
                    childItem.setIcon(QIcon(":folder"))
                    item.appendRow(childItem)

                    projectPath = projectKey + "." + p
                    subConfig = categoryConfig.get(categoryKey)
                    if isinstance(subConfig, VectorHash):
                        # Vector of Hashes
                        for indexConfig in subConfig:
                            self._handleLeafItems(childItem, projectPath, categoryKey, indexConfig, centralHash)
                    else:
                        # Normal Hash
                        self._handleLeafItems(childItem, projectPath, categoryKey, subConfig, centralHash)

        self.endResetModel()


    def selectPath(self, path):
        index = self.findIndex(path)
        if index is None:
            return

        self.selectionModel.select(index, QItemSelectionModel.ClearAndSelect)


    def findIndex(self, path):
        return self._rFindIndex(self.invisibleRootItem(), path)


    def _rFindIndex(self, item, path):
        for i in xrange(item.rowCount()):
            childItem = item.child(i)
            resultItem = self._rFindIndex(childItem, path)
            if resultItem:
                return resultItem
        
        indexPath = item.data(ProjectModel.ITEM_PATH)
        if indexPath == path:
            return item.index()
        return None

