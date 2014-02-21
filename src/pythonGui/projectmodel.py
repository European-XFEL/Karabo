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


from PyQt4.QtCore import Qt
from PyQt4.QtGui import (QIcon, QItemSelectionModel, QStandardItem,
                         QStandardItemModel)


class ProjectModel(QStandardItemModel):

    ITEM_PATH = Qt.UserRole
    ITEM_SERVER_ID = Qt.UserRole + 1
    ITEM_CLASS_ID = Qt.UserRole + 2


    def __init__(self, parent=None):
        super(ProjectModel, self).__init__(parent)

        self.setHorizontalHeaderLabels(["Projects"])
        self.selectionModel = QItemSelectionModel(self)


    def updateData(self, projectHash):
        self.beginResetModel()
        self.clear()
        self.setHorizontalHeaderLabels(["Projects"])

        rootItem = self.invisibleRootItem()

        # Add child items
        for k in projectHash.keys():
            # Project names - toplevel items
            item = QStandardItem(k)
            font = item.font()
            font.setBold(True)
            item.setFont(font)
            item.setIcon(QIcon(":folder"))
            #item.setExpanded(True)
            rootItem.appendRow(item)

            projectConfig = projectHash.get(k)
            for l in projectConfig.keys():
                # Project key

                # Get children
                categoryConfig = projectConfig.get(l)
                for m in categoryConfig.keys():
                    # Categories - sub items
                    childItem = QStandardItem(categoryConfig.getAttribute(m, "label"))
                    childItem.setIcon(QIcon(":folder"))
                    #childItem.setExpanded(True)
                    item.appendRow(childItem)

                    subConfig = categoryConfig.get(m)
                    if subConfig.empty():
                        continue

                    for n in subConfig.keys():
                        leafItem = QStandardItem(n)
                        # TODO: update icon on availability of device
                        leafItem.setIcon(QIcon(":device-instance"))
                        childItem.appendRow(leafItem)

                        deviceId = k + "." + l + "." + m + "." + n
                        leafItem.setData(deviceId, ProjectModel.ITEM_PATH)

                        classConfig = subConfig.get(n)
                        for classId in classConfig.keys():
                            serverId = classConfig.get(classId + ".serverId")
                            # Set server and class ID
                            leafItem.setData(serverId, ProjectModel.ITEM_SERVER_ID)
                            leafItem.setData(classId, ProjectModel.ITEM_CLASS_ID)

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
            return item.index() #self.createIndex(index.row(), 0, index)
        return None

